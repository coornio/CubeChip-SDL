/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Typedefs.hpp"
#include "Concepts.hpp"
#include "Map2D.hpp"

#include <mutex>
#include <shared_mutex>
#include <algorithm>
#include <atomic>

template <typename U> requires std::is_trivially_copyable_v<U>
class TripleBuffer {
	using size_type = std::size_t;
	Map2D<U> mDataBuffer[3u];

	std::atomic<u32> mReadPos{ 0u };
	std::atomic<u32> mWorkPos{ 1u };
	std::atomic<u32> mSwapPos{ 2u };

	mutable std::shared_mutex mReadLock{};
	mutable std::shared_mutex mWorkLock{};

	void mSwapPosIndices() noexcept {
		mReadPos.store((mReadPos.load(std::memory_order_relaxed) + 1u) % 3u, std::memory_order_release);
		mWorkPos.store((mWorkPos.load(std::memory_order_relaxed) + 1u) % 3u, std::memory_order_release);
		mSwapPos.store((mSwapPos.load(std::memory_order_relaxed) + 1u) % 3u, std::memory_order_release);
	}

	/*==================================================================*/

public:
	constexpr TripleBuffer(size_type cols = 1u, size_type rows = 1u) noexcept
		: mDataBuffer{ Map2D<U>(cols, rows), Map2D<U>(cols, rows), Map2D<U>(cols, rows) }
	{}

	void resize(size_type cols, size_type rows) noexcept {
		std::unique_lock read(mReadLock);
		std::unique_lock work(mWorkLock);

		mDataBuffer[0u].resizeClean(cols, rows);
		mDataBuffer[1u].resizeClean(cols, rows);
		mDataBuffer[2u].resizeClean(cols, rows);
	}

	/*==================================================================*/

private:
	inline const Map2D<U>& getReadBuffer() const noexcept {
		return mDataBuffer[mReadPos.load(std::memory_order_acquire)];
	}

public:
	auto copy() const { return Map2D<U>{ getReadBuffer() }; }

	template <typename T>
		requires (sizeof(T) == sizeof(U) && std::is_trivially_copyable_v<T>)
	void read(T* output) const {
		std::shared_lock read(mReadLock);

		std::copy(std::execution::unseq,
			getReadBuffer().begin(), getReadBuffer().end(), output);
	}

	template <IsContiguousContainer T>
		requires MatchingValueType<T, U>
	void read(T& output) const noexcept {
		std::shared_lock read(mReadLock);

		std::copy(std::execution::unseq,
			getReadBuffer().begin(), getReadBuffer().end(), std::data(output));
	}

	/*==================================================================*/

private:
	inline Map2D<U>& getWorkBuffer() noexcept {
		return mDataBuffer[mWorkPos.load(std::memory_order_relaxed)];
	}

public:
	template <typename T, typename Lambda>
	void write(const T* data, size_type N, Lambda&& function) {
		std::unique_lock work(mWorkLock);

		std::transform(std::execution::unseq,
			data, data + N, getWorkBuffer().data(), function);

		mSwapPosIndices();
	}

	template <IsContiguousContainer T>
		requires (sizeof(ValueType<T>) == sizeof(U) && std::is_trivially_copyable_v<ValueType<T>>)
	void write(const T& data) {
		std::unique_lock work(mWorkLock);

		std::copy(std::execution::unseq,
			std::begin(data), std::end(data), getWorkBuffer().data());

		mSwapPosIndices();
	}

	template <IsContiguousContainer T, typename Lambda>
	void write(const T& data, Lambda&& function) {
		std::unique_lock work(mWorkLock);

		std::transform(std::execution::unseq,
			std::begin(data), std::end(data), getWorkBuffer().data(), function);

		mSwapPosIndices();
	}

	template <IsContiguousContainer T, typename Lambda>
	void write(const T& data1, const T& data2, Lambda&& function) {
		std::unique_lock work(mWorkLock);

		std::transform(std::execution::unseq,
			std::begin(data1), std::end(data1), std::begin(data2), getWorkBuffer().data(), function);

		mSwapPosIndices();
	}
};
