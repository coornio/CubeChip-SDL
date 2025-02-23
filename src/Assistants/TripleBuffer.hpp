/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Typedefs.hpp"
#include "Concepts.hpp"
#include "Map2D.hpp"
#include "Aligned.hpp"

#include <mutex>
#include <shared_mutex>
#include <algorithm>
#include <atomic>
#include <iostream>

template <typename U> requires std::is_trivially_copyable_v<U>
class TripleBuffer {
	using Buffer  = Aligned<U, 64>;

	Buffer mDataBuffer[3];

	mutable size_type mSize{};
	mutable bool mBufferIsDirty{};
	mutable std::shared_mutex mSwapLock{};


	alignas(64) mutable Buffer* pWorkBuffer{ &mDataBuffer[0] };
	alignas(64) mutable Buffer* pSwapBuffer{ &mDataBuffer[1] };
	alignas(64) mutable Buffer* pReadBuffer{ &mDataBuffer[2] };
	
	/*==================================================================*/

public:
	constexpr TripleBuffer(size_type size = 0)
		: mDataBuffer{ Buffer(size), Buffer(size), Buffer(size) }
		, mSize(size)
	{}

	constexpr size_type size() const noexcept { return mSize; }

	// DO NOT CALL IF EITHER A CONSUMER OR PRODUCER IS ACTIVE!!
	void resize(size_type buffer_size) {
		if (buffer_size == size()) { return; }

		mSize = buffer_size;
		mDataBuffer[0].reallocate(buffer_size);
		mDataBuffer[1].reallocate(buffer_size);
		mDataBuffer[2].reallocate(buffer_size);
	}

	/*==================================================================*/

private:
	void acquireReadBuffer() const noexcept {
		std::unique_lock lock{ mSwapLock };
		if (mBufferIsDirty) {
			std::swap(pReadBuffer, pSwapBuffer);
			mBufferIsDirty = false;
		}
	}

public:
	Buffer copy(size_type amount) const {
		Buffer temp(amount);
		acquireReadBuffer();

		std::copy_n(std::execution::unseq,
			(*pReadBuffer).data(), std::min(size(), amount), temp.data());

		return temp;
	}

	Buffer copy() const {
		Buffer temp(size());
		acquireReadBuffer();

		std::copy(std::execution::unseq,
			(*pReadBuffer).begin(), (*pReadBuffer).end(), temp.data());

		return temp;
	}

	template <typename T>
		requires (sizeof(T) == sizeof(U) && std::is_trivially_copyable_v<T>)
	void read(T* output, size_type N = 0) const noexcept {
		acquireReadBuffer();
		
		std::copy_n(std::execution::unseq,
			(*pReadBuffer).begin(), std::min(size(), N ? N : size()), output);
	}

	template <IsContiguousContainer T>
		requires MatchingValueType<T, U>
	void read(T& output) const noexcept {
		acquireReadBuffer();
		
		std::copy(std::execution::unseq,
			(*pReadBuffer).begin(), (*pReadBuffer).end(), std::data(output));
	}

	/*==================================================================*/

private:
	void commitWorkerChanges() const noexcept {
		std::unique_lock lock{ mSwapLock };
		std::swap(pWorkBuffer, pSwapBuffer);
		mBufferIsDirty = true;
	}

public:
	template <typename T, typename Lambda>
	void write(const T* data, size_type N, Lambda&& function) {
		std::transform(std::execution::unseq,
			data, data + N, (*pWorkBuffer).data(), function);
		
		commitWorkerChanges();
	}

	template <IsContiguousContainer T>
		requires (sizeof(ValueType<T>) == sizeof(U) && std::is_trivially_copyable_v<ValueType<T>>)
	void write(const T& data) {
		std::copy(std::execution::unseq,
			std::begin(data), std::end(data), (*pWorkBuffer).data());
		
		commitWorkerChanges();
	}

	template <IsContiguousContainer T, typename Lambda>
	void write(const T& data, Lambda&& function) {
		std::transform(std::execution::unseq,
			std::begin(data), std::end(data), (*pWorkBuffer).data(), function);
		
		commitWorkerChanges();
	}

	template <IsContiguousContainer T, typename Lambda>
	void write(const T& data1, const T& data2, Lambda&& function) {
		std::transform(std::execution::unseq,
			std::begin(data1), std::end(data1), std::begin(data2), (*pWorkBuffer).data(), function);
		
		commitWorkerChanges();
	}
};
