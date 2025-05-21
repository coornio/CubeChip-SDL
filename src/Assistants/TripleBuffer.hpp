/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Typedefs.hpp"
#include "Concepts.hpp"
#include "Aligned.hpp"

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4324)
#endif

#if false
	#define USE_MUTEX
#endif

#define USE_MUTEX

#if defined(USE_MUTEX)
	#include <mutex>
	#include <shared_mutex>
#endif
	#include <memory>
	#include <atomic>
	#include <algorithm>

/*==================================================================*/

template <typename U>
	requires (std::is_trivially_copyable_v<U>)
class TripleBuffer {
	using Buffer = AlignedUnique<U>;

	alignas(HDIS) Buffer pWorkBuffer;
	alignas(HDIS) Buffer pSwapBuffer;
	alignas(HDIS) Buffer pReadBuffer;

	alignas(HDIS) std::shared_mutex mSwapLock{};
	              std::atomic<bool> mBufferIsDirty{};
	              std::size_t mSize{};
	
	/*==================================================================*/

public:
	constexpr TripleBuffer(std::size_t buffer_size = 0)
		: pWorkBuffer{ ::allocate<U>(buffer_size).as_value() }
		, pSwapBuffer{ ::allocate<U>(buffer_size).as_value() }
		, pReadBuffer{ ::allocate<U>(buffer_size).as_value() }
		, mSize(pWorkBuffer && pSwapBuffer && pReadBuffer ? buffer_size : 0)
	{}

	constexpr TripleBuffer(const TripleBuffer&)  = delete;
	constexpr TripleBuffer(TripleBuffer&&)       = delete;
	TripleBuffer& operator=(const TripleBuffer&) = delete;
	TripleBuffer& operator=(TripleBuffer&&)      = delete;

	constexpr auto size() const noexcept { return mSize; }

	// DO NOT CALL IF EITHER A CONSUMER OR PRODUCER IS ACTIVE!!
	void resize(std::size_t buffer_size) {
		mSize = buffer_size;
		pWorkBuffer.reset(); pWorkBuffer = ::allocate<U>(buffer_size).as_value();
		pSwapBuffer.reset(); pSwapBuffer = ::allocate<U>(buffer_size).as_value();
		pReadBuffer.reset(); pReadBuffer = ::allocate<U>(buffer_size).as_value();
		assert(pWorkBuffer && pSwapBuffer && pReadBuffer);
	}

	/*==================================================================*/

private:
	void acquireReadBuffer() noexcept {
		if (mBufferIsDirty.load(mo::acquire)) {
			std::unique_lock lock{ mSwapLock };
			std::swap(pReadBuffer, pSwapBuffer);
			mBufferIsDirty.store(false, mo::release);
		}
	}

public:
	Buffer copy(std::size_t amount) {
		Buffer temp(amount);
		acquireReadBuffer();

		std::copy_n(EXEC_POLICY(unseq)
			pReadBuffer.get(), std::min(size(), amount), temp.data());

		return temp;
	}

	Buffer copy() {
		Buffer temp(size());
		acquireReadBuffer();

		std::copy_n(EXEC_POLICY(unseq)
			pReadBuffer.get(), size(), temp.data());

		return temp;
	}

	template <typename T>
		requires (sizeof(T) == sizeof(U) && std::is_trivially_copyable_v<T>)
	void read(T* output, std::size_t N = 0) noexcept {
		acquireReadBuffer();
		
		std::copy_n(EXEC_POLICY(unseq)
			pReadBuffer.get(), std::min(size(), N ? N : size()), output);
	}

	template <IsContiguousContainer T>
		requires MatchingValueType<T, U>
	void read(T& output) noexcept {
		acquireReadBuffer();
		
		std::copy(EXEC_POLICY(unseq)
			pReadBuffer.get(), pReadBuffer.get() + mSize, std::data(output));
	}

	/*==================================================================*/

private:
	void commitWorkerChanges() noexcept {
		std::swap(pWorkBuffer, pSwapBuffer);
		mBufferIsDirty.store(true, mo::release);
	}

public:
	template <typename T, typename Lambda>
	void write(const T* data, std::size_t N, Lambda&& function) {
		std::unique_lock lock{ mSwapLock };
		std::transform(EXEC_POLICY(unseq)
			data, data + N, pWorkBuffer.get(), function);
		
		commitWorkerChanges();
	}

	template <IsContiguousContainer T>
		requires (sizeof(ValueType<T>) == sizeof(U) && std::is_trivially_copyable_v<ValueType<T>>)
	void write(const T& data) {
		std::unique_lock lock{ mSwapLock };
		std::copy(EXEC_POLICY(unseq)
			std::begin(data), std::end(data), pWorkBuffer.get());
		
		commitWorkerChanges();
	}

	template <IsContiguousContainer T, typename Lambda>
	void write(const T& data, Lambda&& function) {
		std::unique_lock lock{ mSwapLock };
		std::transform(EXEC_POLICY(unseq)
			std::begin(data), std::end(data), pWorkBuffer.get(), function);
		
		commitWorkerChanges();
	}

	template <IsContiguousContainer T, typename Lambda>
	void write(const T& data1, const T& data2, Lambda&& function) {
		std::unique_lock lock{ mSwapLock };
		std::transform(EXEC_POLICY(unseq)
			std::begin(data1), std::end(data1), std::begin(data2), pWorkBuffer.get(), function);
		
		commitWorkerChanges();
	}
};

#ifdef _MSC_VER
	#pragma warning(pop)
#endif
