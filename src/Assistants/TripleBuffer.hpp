/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Typedefs.hpp"
#include "Concepts.hpp"
#include "Map2D.hpp"

#include <mutex>
#include <shared_mutex>
#include <algorithm>
#include <atomic>

template <typename U> requires std::is_trivially_copyable_v<U>
class TripleBuffer {
	Map2D<U> mDataBuffer[3u];

	std::atomic<u32> mReadPos{ 0u };

	mutable std::shared_mutex mReadLock{};
	        std::shared_mutex mWorkLock{};

	/*==================================================================*/

public:
	constexpr TripleBuffer(size_type cols = 1u, size_type rows = 1u) noexcept
		: mDataBuffer{ Map2D<U>(cols, rows), Map2D<U>(cols, rows), Map2D<U>(cols, rows) }
	{}

	void resize(size_type cols, size_type rows) noexcept {
		std::unique_lock read{ mReadLock };
		std::unique_lock work{ mWorkLock };

		mDataBuffer[0u].resizeClean(cols, rows);
		mDataBuffer[1u].resizeClean(cols, rows);
		mDataBuffer[2u].resizeClean(cols, rows);
	}

	/*==================================================================*/

private:
	inline const auto& getReadBuffer() const noexcept {
		return mDataBuffer[mReadPos.load(mo::acquire)];
	}

public:
	Map2D<U> copy() const { return getReadBuffer(); }

	template <typename T>
		requires (sizeof(T) == sizeof(U) && std::is_trivially_copyable_v<T>)
	void read(T* output) const {
		std::unique_lock read{ mReadLock };

		std::copy(std::execution::unseq,
			getReadBuffer().begin(), getReadBuffer().end(), output);
	}

	template <IsContiguousContainer T>
		requires MatchingValueType<T, U>
	void read(T& output) const noexcept {
		std::unique_lock read{ mReadLock };

		std::copy(std::execution::unseq,
			getReadBuffer().begin(), getReadBuffer().end(), std::data(output));
	}

	/*==================================================================*/

private:
	inline auto* getWorkBufferData() const noexcept {
		return mDataBuffer[(mReadPos.load(mo::relaxed) + 1u) % 3u].data();
	}

	inline void incrementReaderPos() noexcept {
		mReadPos.store((mReadPos.load(mo::seq_cst) + 1u) % 3u, mo::release);
	}

public:
	template <typename T, typename Lambda>
	void write(const T* data, size_type N, Lambda&& function) {
		std::unique_lock work{ mWorkLock };

		std::transform(std::execution::unseq,
			data, data + N, getWorkBufferData(), function);

		incrementReaderPos();
	}

	template <IsContiguousContainer T>
		requires (sizeof(ValueType<T>) == sizeof(U) && std::is_trivially_copyable_v<ValueType<T>>)
	void write(const T& data) {
		std::unique_lock work{ mWorkLock };

		std::copy(std::execution::unseq,
			std::begin(data), std::end(data), getWorkBufferData());

		incrementReaderPos();
	}

	template <IsContiguousContainer T, typename Lambda>
	void write(const T& data, Lambda&& function) {
		std::unique_lock work{ mWorkLock };

		std::transform(std::execution::unseq,
			std::begin(data), std::end(data), getWorkBufferData(), function);

		incrementReaderPos();
	}

	template <IsContiguousContainer T, typename Lambda>
	void write(const T& data1, const T& data2, Lambda&& function) {
		std::unique_lock work{ mWorkLock };

		std::transform(std::execution::unseq,
			std::begin(data1), std::end(data1), std::begin(data2), getWorkBufferData(), function);

		incrementReaderPos();
	}
};
