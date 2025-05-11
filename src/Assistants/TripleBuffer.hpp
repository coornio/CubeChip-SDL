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

#if defined(USE_MUTEX)
	#include <mutex>
	#include <shared_mutex>
#else
	#include <atomic>
#endif
	#include <algorithm>

/*==================================================================*/

template <typename U>
	requires (std::is_trivially_copyable_v<U>)
class TripleBuffer {
	using Buffer = Aligned<U>;

	Buffer mDataBuffer[3];

	mutable size_type mSize{};

#if defined(USE_MUTEX)
	mutable bool mBufferIsDirty{};
	mutable std::shared_mutex mSwapLock{};

	alignas(HDIS) mutable Buffer* pSwapBuffer { &mDataBuffer[1] };
#else
	alignas(HDIS) mutable Atom<std::uintptr_t> pSwapBuffer
		{ reinterpret_cast<std::uintptr_t>(&mDataBuffer[1]) };
#endif

	alignas(HDIS) mutable Buffer* pWorkBuffer{ &mDataBuffer[0] };
	alignas(HDIS) mutable Buffer* pReadBuffer{ &mDataBuffer[2] };
	
	/*==================================================================*/

public:
	constexpr TripleBuffer(size_type size = 0)
		: mDataBuffer{ Buffer(size), Buffer(size), Buffer(size) }
		, mSize(size)
	{}

	constexpr TripleBuffer(const TripleBuffer&)  = delete;
	constexpr TripleBuffer(TripleBuffer&&)       = delete;
	TripleBuffer& operator=(const TripleBuffer&) = delete;
	TripleBuffer& operator=(TripleBuffer&&)      = delete;

	constexpr size_type size() const noexcept { return mSize; }

	// DO NOT CALL IF EITHER A CONSUMER OR PRODUCER IS ACTIVE!!
	void resize(size_type buffer_size) {
		mSize = buffer_size;
		mDataBuffer[0].reallocate(buffer_size); assert(mDataBuffer[0].size());
		mDataBuffer[1].reallocate(buffer_size); assert(mDataBuffer[1].size());
		mDataBuffer[2].reallocate(buffer_size); assert(mDataBuffer[2].size());
	}

	/*==================================================================*/

private:
	void acquireReadBuffer() const noexcept {
	#if defined(USE_MUTEX)
		std::unique_lock lock{ mSwapLock };
		if (mBufferIsDirty) {
			std::swap(pReadBuffer, pSwapBuffer);
			mBufferIsDirty = false;
		}
	#else
		if (pSwapBuffer.load(mo::relaxed) & 0x1ull) {
			auto prev{ pSwapBuffer.exchange(reinterpret_cast<std::uintptr_t>(pReadBuffer), mo::acq_rel) };
			pReadBuffer = reinterpret_cast<Buffer*>(prev & ~0x1ull);
		}
	#endif
	}

public:
	Buffer copy(size_type amount) const {
		Buffer temp(amount);
		acquireReadBuffer();

		std::copy_n(EXEC_POLICY(unseq)
			(*pReadBuffer).data(), std::min(size(), amount), temp.data());

		return temp;
	}

	Buffer copy() const {
		Buffer temp(size());
		acquireReadBuffer();

		std::copy(EXEC_POLICY(unseq)
			(*pReadBuffer).begin(), (*pReadBuffer).end(), temp.data());

		return temp;
	}

	template <typename T>
		requires (sizeof(T) == sizeof(U) && std::is_trivially_copyable_v<T>)
	void read(T* output, size_type N = 0) const noexcept {
		acquireReadBuffer();
		
		std::copy_n(EXEC_POLICY(unseq)
			(*pReadBuffer).begin(), std::min(size(), N ? N : size()), output);
	}

	template <IsContiguousContainer T>
		requires MatchingValueType<T, U>
	void read(T& output) const noexcept {
		acquireReadBuffer();
		
		std::copy(EXEC_POLICY(unseq)
			(*pReadBuffer).begin(), (*pReadBuffer).end(), std::data(output));
	}

	/*==================================================================*/

private:
	void commitWorkerChanges() const noexcept {
	#if defined(USE_MUTEX)
		std::unique_lock lock{ mSwapLock };
		std::swap(pWorkBuffer, pSwapBuffer);
		mBufferIsDirty = true;
	#else
		const auto dirty{ 0x1ull | reinterpret_cast<std::uintptr_t>(pWorkBuffer) };
		auto prev{ pSwapBuffer.exchange(dirty, mo::acq_rel) & ~0x1ull };
		pWorkBuffer = reinterpret_cast<Buffer*>(prev);
	#endif
	}

public:
	template <typename T, typename Lambda>
	void write(const T* data, size_type N, Lambda&& function) {
		std::transform(EXEC_POLICY(unseq)
			data, data + N, (*pWorkBuffer).data(), function);
		
		commitWorkerChanges();
	}

	template <IsContiguousContainer T>
		requires (sizeof(ValueType<T>) == sizeof(U) && std::is_trivially_copyable_v<ValueType<T>>)
	void write(const T& data) {
		std::copy(EXEC_POLICY(unseq)
			std::begin(data), std::end(data), (*pWorkBuffer).data());
		
		commitWorkerChanges();
	}

	template <IsContiguousContainer T, typename Lambda>
	void write(const T& data, Lambda&& function) {
		std::transform(EXEC_POLICY(unseq)
			std::begin(data), std::end(data), (*pWorkBuffer).data(), function);
		
		commitWorkerChanges();
	}

	template <IsContiguousContainer T, typename Lambda>
	void write(const T& data1, const T& data2, Lambda&& function) {
		std::transform(EXEC_POLICY(unseq)
			std::begin(data1), std::end(data1), std::begin(data2), (*pWorkBuffer).data(), function);
		
		commitWorkerChanges();
	}
};

#ifdef _MSC_VER
	#pragma warning(pop)
#endif
