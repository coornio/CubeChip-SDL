/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <array>
#include <mutex>
#include <shared_mutex>
#include <utility>

#include "Typedefs.hpp"

/*==================================================================*/

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4324)
#endif

/**
 * @brief A lock-free, multi-producer, multi-consumer ring buffer.
 * @tparam T :: The type stored in the buffer. Must be default constructible.
 * @tparam N :: The number of slots in the buffer. Must be a power of two, and at least 8.
 * @details This ring buffer allows concurrent push and read operations from multiple threads.
 *          Internally uses atomic operations and shared pointers to manage lifetime.
 * @note Values passed to the push() method must be same as or convertible to `T`.
 * @code{.cpp}
 *  SimpleRingBuffer<std::string, 256> buffer;
 *  buffer.push("hello");
 * @endcode
 */
template <typename T, std::size_t N = 8>
	requires (std::is_default_constructible_v<T>)
class SimpleRingBuffer {
	static_assert((N & (N - 1)) == 0, "N must be a power of two.");
	static_assert(N >= 8, "N must be at least 8.");

	using self = SimpleRingBuffer;

	alignas(HDIS) std::array<AtomSharedPtr<T>, N> mBuffer{};
	alignas(HDIS) std::atomic<size_type>          mPushHead{ 0 };
	alignas(HDIS) std::atomic<size_type>          mReadHead{ 0 };
	mutable       std::shared_mutex               mGuard;

public:
	SimpleRingBuffer() noexcept = default;

	SimpleRingBuffer(self&&) = delete;
	SimpleRingBuffer(const self&) = delete;

	self& operator=(self&&) = delete;
	self& operator=(const self&) = delete;

	constexpr auto size() const noexcept { return N; }
	constexpr auto head() const noexcept { return mReadHead.load(mo::acquire); }

private:
	void push_(size_type index, std::shared_ptr<T>&& ptr) noexcept {
		std::shared_lock lock{ mGuard };
		mBuffer[index & (N - 1)].store(std::move(ptr), mo::release);

		auto expected{ head() };
		while (expected < index && !mReadHead.compare_exchange_weak \
			(expected, index, mo::acq_rel, mo::acquire)) {}
	}

	auto at_(size_type index, size_type head) const noexcept {
		const auto pos{ (head + N - index) & (N - 1) };
		const auto ptr{ mBuffer[pos].load(mo::acquire) };
		return ptr ? *ptr : T{};
	}

	auto snapshot_() const noexcept {
		std::array<T, N> output;
		const auto pos{ head() & (N - 1) };

		std::for_each(EXEC_POLICY(unseq)
			output.begin(), output.end(),
			[&](auto& entry) noexcept { entry = std::move(
				at_((N - 1) - (&entry - output.data()), pos)
			); }
		);

		return output;
	}

public:
	/**
	 * @brief Push a new value into the internal buffer. Each call advances the relative index
	 *        used by at() calls in a monotonic fashion.
	 * @param[in] value :: Value to copy or move into the internal buffer.
	 * @note This method is thread-safe, but cannot run concurrently with safe_snapshot() calls.
	 */
	template <typename U>
		requires (std::is_convertible_v<U, T>)
	void push(U&& value) {
		push_(
			mPushHead.fetch_add(1, mo::acq_rel),
			std::make_shared<T>(std::forward<U>(value))
		);
	}

	/**
	 * @brief Retrieve a copy of a value from the internal buffer. The index is relative to the 
	 *        the most recent push() call, with 0 being the most recent entry.
	 * @param[in] index :: Offset relative to the most recent entry.
	 * @note This method is thread-safe, but may return stale data due to its non-blocking nature.
	 */
	T at(size_type index) const noexcept {
		return at_(index, head());
	}

	/**
	 * @brief Create a non-blocking snapshot of the internal buffer's contents. Values are ordered
	 *        from oldest to newest at the time of the call, from 0 to N-1, respectively.
	 * @returns A vector of T representing the buffer contents; missing values are default-constructed.
	 * @note This method is thread-safe, but may return stale data due to its non-blocking nature.
	 */
	std::array<T, N> fast_snapshot() const noexcept {
		return snapshot_();
	}

	/**
	 * @brief Create a blocking snapshot of the internal buffer's contents. Values are ordered
	 *        from oldest to newest at the time of the call, from 0 to N-1, respectively.
	 * @returns A vector of T representing the buffer contents; missing values are default-constructed.
	 * @note This method is thread-safe, but cannot run concurrently with push() calls.
	 */
	std::array<T, N> safe_snapshot() const noexcept {
		std::unique_lock lock{ mGuard };
		return snapshot_();
	}
};

#ifdef _MSC_VER
	#pragma warning(pop)
#endif
