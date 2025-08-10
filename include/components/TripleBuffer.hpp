/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

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

// TripleBuffer is a thread-safe buffer that allows for concurrent reading and writing
// by using a work buffer, a swap buffer, and a read buffer. The work buffer is used
// for writing data, the swap buffer is used to swap the work buffer with the read buffer,
// and the read buffer is used for reading data. The swap is protected by a shared mutex
// to ensure that only one thread can swap the buffers at a time, while allowing multiple
// threads to read from the read buffer concurrently.
// Access to read() or write() does not reserve the associated buffer, so there are no lock
// or release calls required for either process, allowing single-call operations. Read fully
// and write fully without worries. Same applies to copy().
template <typename U>
	requires (std::is_trivially_copyable_v<U>)
class TripleBuffer {
	using Buffer = AlignedUniqueArray<U>;
	using mo = std::memory_order;

	alignas(HDIS) Buffer pWorkBuffer;
	alignas(HDIS) Buffer pSwapBuffer;
	alignas(HDIS) Buffer pReadBuffer;

	alignas(HDIS) std::shared_mutex mSwapLock{};
	              std::atomic<bool> mBufferIsDirty{};
	              std::size_t mSize{};
	
	/*==================================================================*/

public:
	constexpr TripleBuffer(std::size_t buffer_size = 0)
		: pWorkBuffer{ ::allocate_n<U>(buffer_size).as_value().release() }
		, pSwapBuffer{ ::allocate_n<U>(buffer_size).as_value().release() }
		, pReadBuffer{ ::allocate_n<U>(buffer_size).as_value().release() }
		, mSize(pWorkBuffer && pSwapBuffer && pReadBuffer ? buffer_size : 0)
	{}

	constexpr TripleBuffer(const TripleBuffer&)  = delete;
	constexpr TripleBuffer(TripleBuffer&&)       = delete;
	TripleBuffer& operator=(const TripleBuffer&) = delete;
	TripleBuffer& operator=(TripleBuffer&&)      = delete;

	constexpr auto size() const noexcept { return mSize; }

	// DO NOT CALL IF EITHER A CONSUMER OR PRODUCER IS ACTIVE!!
	// This will reset the buffers to a new size, which will invalidate any
	// pointers or references to the previous buffers, causing undefined behavior.
	void resize(std::size_t buffer_size) {
		mSize = buffer_size;
		pWorkBuffer.reset(); pWorkBuffer = ::allocate_n<U>(buffer_size).as_value().release();
		pSwapBuffer.reset(); pSwapBuffer = ::allocate_n<U>(buffer_size).as_value().release();
		pReadBuffer.reset(); pReadBuffer = ::allocate_n<U>(buffer_size).as_value().release();
		assert(pWorkBuffer && pSwapBuffer && pReadBuffer);
	}

	/*==================================================================*/

private:
	U* acquireReadBuffer() noexcept {
		if (mBufferIsDirty.load(mo::acquire)) {
			std::unique_lock lock{ mSwapLock };
			std::swap(pReadBuffer, pSwapBuffer);
			mBufferIsDirty.store(false, mo::release);
		}
		return pReadBuffer.get();
	}

public:
	// Copy the contents of the TripleBuffer to a new Buffer object and return it.
	Buffer copy(std::size_t N = 0u) {
		const auto new_count{ std::min(size(), N ? N : size()) };
		auto temp{ ::allocate_n<U>(new_count).by_copy(acquireReadBuffer(), new_count).as_value() };
		return temp.is_constructed() ? temp.release() : Buffer{};
	}

	// Copy the contents of the TripleBuffer to a given array via raw pointer.
	template <typename T>
		requires (sizeof(T) == sizeof(U) && std::is_trivially_copyable_v<T>)
	void read(T* output, std::size_t N = 0) noexcept {
		std::copy_n(EXEC_POLICY(unseq)
			acquireReadBuffer(), std::min(size(), N ? N : size()), output);
	}

	// Copy the contents of the TripleBuffer to a given contiguous container.
	template <IsContiguousContainer T>
		requires MatchingValueType<T, U>
	void read(T& output) noexcept {
		std::copy_n(EXEC_POLICY(unseq)
			acquireReadBuffer(), size(), std::data(output));
	}

	/*==================================================================*/

private:
	void commitWorkerChanges() noexcept {
		std::swap(pWorkBuffer, pSwapBuffer);
		mBufferIsDirty.store(true, mo::release);
	}

public:
	// Write to the TripleBuffer by copying data from a raw pointer.
	template <typename T, typename Lambda>
	void write(const T* data, std::size_t N, Lambda&& function) {
		std::unique_lock lock{ mSwapLock };
		std::transform(EXEC_POLICY(unseq)
			data, data + N, pWorkBuffer.get(), function);
		
		commitWorkerChanges();
	}

	// Write to the TripleBuffer by copying data from a contiguous container.
	template <IsContiguousContainer T>
		requires (sizeof(ValueType<T>) == sizeof(U) && std::is_trivially_copyable_v<ValueType<T>>)
	void write(const T& data) {
		std::unique_lock lock{ mSwapLock };
		std::copy(EXEC_POLICY(unseq)
			std::begin(data), std::end(data), pWorkBuffer.get());
		
		commitWorkerChanges();
	}

	// Write to the TripleBuffer by applying a unary function to each element of a contiguous container.
	template <IsContiguousContainer T, typename Lambda>
	void write(const T& data, Lambda&& function) {
		std::unique_lock lock{ mSwapLock };
		std::transform(EXEC_POLICY(unseq)
			std::begin(data), std::end(data), pWorkBuffer.get(), function);
		
		commitWorkerChanges();
	}

	// Write to the TripleBuffer by applying a binary function to each element from two contiguous containers.
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
