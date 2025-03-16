/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <span>
#include <execution>
#include <exception>
#include <cstdlib>
#include <cassert>
#include <memory>

#include "RangeIterator.hpp"

template <typename T>
	requires (std::is_default_constructible_v<T>)
class Aligned {

public:
	using element_type    = T;
	using axis_size       = std::uint32_t;
	using size_type       = std::size_t;
	using difference_type = std::ptrdiff_t;
	using value_type      = std::remove_cv_t<T>;

	using pointer       = T*;
	using const_pointer = const T*;

	using reference       = T&;
	using const_reference = const T&;

	using iterator       = T*;
	using const_iterator = const T*;

	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
	struct Deleter {
		void operator()(T* ptr) const noexcept {
			::operator delete[](ptr, std::align_val_t(HDIS));
		}
	};

	std::unique_ptr<T[], Deleter> pData{};
	size_type mSize{};

public:
	Aligned(size_type size = 0) noexcept
		: mSize{ size }
	{
		if (!size) { return; }
		size = (size + (HDIS - 1)) & ~(HDIS - 1);
		
		auto ptr{ static_cast<pointer>(::operator new[](
			size * sizeof(T), std::align_val_t(HDIS), std::nothrow)) };
		assert(ptr); pData.reset(ptr);

		if (!ptr) { mSize = 0; } else {
			if constexpr (std::is_trivially_default_constructible_v<T>) {
				std::uninitialized_value_construct_n(EXEC_POLICY(unseq) ptr, size);
			} else {
				std::uninitialized_default_construct_n(EXEC_POLICY(unseq) ptr, size);
			}
		}
	}

	void initialize(const value_type& value = value_type()) noexcept {
		std::fill(EXEC_POLICY(unseq)
			begin(), end(), value);
	}

	void resize(size_type size) noexcept {
		if (size == mSize) { return; }

		Aligned newAligned(size);
		if (newAligned.size() != size) { return; }

		if constexpr (std::is_trivially_copyable_v<T>) {
			std::copy(EXEC_POLICY(unseq)
				data(), data() + std::min(mSize, size), newAligned.data());
		} else {
			std::move(EXEC_POLICY(unseq)
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

public:
	constexpr pointer   data()  const { return pData.get(); }
	constexpr reference front() const { return data()[0]; }
	constexpr reference back()  const { return data()[size() - 1]; }

	constexpr size_type size()       const noexcept { return mSize; }
	constexpr size_type size_bytes() const noexcept { return size() * sizeof(value_type); }
	constexpr bool      empty()      const noexcept { return size() == 0; }
	constexpr auto      span()       const noexcept { return std::span(data(), size()); }

	constexpr auto first(size_type count) const { return RangeProxy(data(), count); }
	constexpr auto last(size_type count)  const { return RangeProxy(data(), size() - count); }

	// cast underlying data to a RangeProxy (span) for a lot of added functionality
	constexpr auto operator*() const noexcept { return RangeProxy(data(), mSize); }

	explicit constexpr operator bool() const noexcept { return static_cast<bool>(pData); }

public:
	constexpr reference at(size_type idx) const {
		if (idx >= size()) { throw std::out_of_range("Aligned.at() index out of range"); }
		return data()[idx];
	}
	constexpr reference operator[](size_type idx) const {
		assert(idx < size() && "Aligned.operator[] index out of bounds");
		return data()[idx];
	}

public:
	constexpr iterator begin() const noexcept { return data(); }
	constexpr iterator end()   const noexcept { return data() + size(); }

	constexpr reverse_iterator rbegin() const noexcept { return std::make_reverse_iterator(end()); }
	constexpr reverse_iterator rend()   const noexcept { return std::make_reverse_iterator(begin()); }

	constexpr const_iterator cbegin() const noexcept { return begin(); }
	constexpr const_iterator cend()   const noexcept { return end(); }

	constexpr const_reverse_iterator crbegin() const noexcept { return std::make_reverse_iterator(cend()); }
	constexpr const_reverse_iterator crend()   const noexcept { return std::make_reverse_iterator(cbegin()); }
};
