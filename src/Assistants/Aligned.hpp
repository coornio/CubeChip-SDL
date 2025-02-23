/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <mutex>
#include <shared_mutex>
#include <execution>
#include <cstdlib>
#include <cassert>
#include <memory>

#include "Typedefs.hpp"
#include "RangeIterator.hpp"

template <typename T, size_type Alignment = alignof(T)>
	requires std::is_default_constructible_v<T>
class Aligned {
	static_assert(Alignment >= alignof(T), "Alignment must be at least alignof(T)");
	static_assert((Alignment & (Alignment - 1)) == 0, "Alignment must be a power of 2");

	using value_type = std::remove_cv_t<T>;

	struct Deleter {
		void operator()(T* ptr) const noexcept {
			::operator delete[](ptr, std::align_val_t(Alignment));
		}
	};

	std::unique_ptr<T[], Deleter> pData{};
	size_type mSize{};

public:
	Aligned(size_type size = 0) : mSize{ size } {
		if (!size) { return; }
		size = (size + (Alignment - 1)) & ~(Alignment - 1);
		
		T* ptr{ static_cast<T*>(::operator new[](
			size * sizeof(T), std::align_val_t(Alignment), std::nothrow)) };
		assert(ptr); pData.reset(ptr);

		if (ptr) {
			if constexpr (std::is_trivially_default_constructible_v<T>) {
				std::uninitialized_value_construct_n(std::execution::unseq, ptr, size);
			} else {
				std::uninitialized_default_construct_n(std::execution::unseq, ptr, size);
			}
		}
	}

	void resize(size_type size) {
		if (size == mSize) { return; }

		Aligned newAligned(size);

		if constexpr (std::is_trivially_copyable_v<T>) {
			std::copy(std::execution::unseq,
				data(), data() + std::min(mSize, size), newAligned.data());
		} else {
			std::move(std::execution::unseq,
				data(), data() + std::min(mSize, size), newAligned.data());
		}

		*this = std::move(newAligned);
	}

	void reallocate(size_type size) {
		*this = std::move(Aligned(size));
	}

	Aligned(const Aligned&)            = delete;
	Aligned& operator=(const Aligned&) = delete;

	Aligned(Aligned&&)            noexcept = default;
	Aligned& operator=(Aligned&&) noexcept = default;

	constexpr T*   data() const noexcept { return pData.get(); }
	constexpr auto size() const noexcept { return mSize; }

	constexpr auto begin() const noexcept { return data(); }
	constexpr auto end()   const noexcept { return data() + size(); }

	// cast underlying data to a RangeProxy (span) for a lot of added functionality
	constexpr auto operator*() const noexcept { return RangeProxy(data(), mSize); }

	explicit constexpr operator bool() const noexcept { return static_cast<bool>(pData); }
};
