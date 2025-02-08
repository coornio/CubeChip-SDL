/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Typedefs.hpp"
#include "Concepts.hpp"
#include "Map2D.hpp"

#include <shared_mutex>
#include <atomic>

template <typename T>
class TripleBuffer {
	using size_type = std::size_t;
	Map2D<T> dataBuffer[3u];

	std::atomic<u32> readBuffer{ 0u };
	std::atomic<u32> workBuffer{ 1u };
	std::atomic<u32> swapBuffer{ 2u };
	std::shared_mutex reader{};
	std::shared_mutex worker{};

	void swapBufferIndices() noexcept {
		readBuffer.store((readBuffer.load(std::memory_order_relaxed) + 1u) % 3u, std::memory_order_release);
		workBuffer.store((workBuffer.load(std::memory_order_relaxed) + 1u) % 3u, std::memory_order_release);
		swapBuffer.store((swapBuffer.load(std::memory_order_relaxed) + 1u) % 3u, std::memory_order_release);
	}

public:
	constexpr TripleTextureBuffer(size_type cols, size_type rows) noexcept
		: dataBuffer{ Map2D<T>(cols, rows), Map2D<T>(cols, rows), Map2D<T>(cols, rows) }
	{}

	void resize(size_type cols, size_type rows) noexcept {
		std::unique_lock lock1(reader);
		std::unique_lock lock2(worker);

		dataBuffer[0u].resizeClean(cols, rows);
		dataBuffer[1u].resizeClean(cols, rows);
		dataBuffer[2u].resizeClean(cols, rows);
	}

	auto read() const noexcept {
		std::shared_lock lock(reader);

		return Map2D<T>(dataBuffer[readBuffer.load(std::memory_order_acquire)]);
	}

	template <IsContiguousContainer T>
	void write(const T& pixelData) {
		std::unique_lock lock(worker);

		std::copy(std::execution::unseq,
			pixelData.begin(), pixelData.end(),
			dataBuffer[workBuffer.load(std::memory_order_relaxed)].data());

		swapBufferIndices();
	}
};
