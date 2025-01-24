/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Concepts.hpp"

#include <cstddef>
#include <stdexcept>

#pragma region RangeProxy Class
template <typename T>
struct RangeProxy final {
	using element_type    = T;
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

protected:
	pointer   mData;
	size_type mSize;

public:
	constexpr RangeProxy(pointer data, size_type length) noexcept
		: mData { data  }, mSize{ length }
	{}

	template <size_type N>
	constexpr RangeProxy(T(&array)[N]) noexcept
		: mData{ array }, mSize{ N }
	{}

	template <IsContiguousContainer Object>
	constexpr RangeProxy(Object& array) noexcept
		: mData{ std::data(array) }, mSize{ std::size(array) }
	{}

	constexpr size_type size()       const noexcept { return mSize; }
	constexpr size_type size_bytes() const noexcept { return size() * sizeof(value_type); }
	constexpr bool      empty()      const noexcept { return size() == 0; }

	constexpr pointer   data()  const { return mData; }
	constexpr reference front() const { return data()[0]; }
	constexpr reference back()  const { return data()[size() - 1]; }

	constexpr RangeProxy first(size_type count) const { return RangeProxy(data(), count); }
	constexpr RangeProxy last(size_type count)  const { return RangeProxy(data(), size() - count); }

	constexpr reference operator[](size_type idx) const { return data()[idx]; }
	constexpr reference at(size_type idx) const {
		if (idx >= size()) {
			throw std::out_of_range("RangeProxy.at() :: Index out of range.");
		}
		return data()[idx];
	}

	constexpr iterator begin() const noexcept { return data(); }
	constexpr iterator end()   const noexcept { return data() + size(); }

	constexpr reverse_iterator rbegin() const noexcept { return end() - 1; }
	constexpr reverse_iterator rend()   const noexcept { return begin() - 1; }

	constexpr const_iterator cbegin() const noexcept { return begin(); }
	constexpr const_iterator cend()   const noexcept { return end(); }

	constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
	constexpr const_reverse_iterator crend()   const noexcept { return rend(); }
};
#pragma endregion

#pragma region RangeIterator Class
template <typename T>
struct RangeIterator final {
	using element_type    = T;
	using value_type      = std::remove_cv_t<T>;
	using size_type       = std::size_t;
	using difference_type = std::ptrdiff_t;

	using iterator_category = std::contiguous_iterator_tag;

	using pointer        = RangeProxy<T>*;
	using const_pointer  = const RangeProxy<T>*;

	using reference       = RangeProxy<T>&;
	using const_reference = const RangeProxy<T>&;

protected:
	RangeProxy<T> mRange;

public:
	constexpr RangeIterator(T* begin, size_type length) noexcept
		: mRange{ begin, length }
	{}

	template <size_type N>
	explicit constexpr RangeIterator(T(&array)[N]) noexcept
		: mRange{ array, N }
	{}

	template <IsContiguousContainer Object>
	explicit constexpr RangeIterator(Object& array) noexcept
		: mRange{ std::data(array), std::size(array) }
	{}

public:
	constexpr auto& operator* () const noexcept { return mRange; }
	constexpr auto* operator->() const noexcept { return &mRange; }

	constexpr auto& operator++() noexcept { mRange.data() += mRange.size(); return *this; }
	constexpr auto& operator--() noexcept { mRange.data() -= mRange.size(); return *this; }

	constexpr auto& operator+=(difference_type rhs) noexcept { mRange.data() += rhs * mRange.size(); return *this; }
	constexpr auto& operator-=(difference_type rhs) noexcept { mRange.data() -= rhs * mRange.size(); return *this; }

	constexpr auto  operator++(int) noexcept { auto tmp{ *this }; mRange.data() += mRange.size(); return tmp; }
	constexpr auto  operator--(int) noexcept { auto tmp{ *this }; mRange.data() -= mRange.size(); return tmp; }

	constexpr auto  operator+ (difference_type rhs) const noexcept { return RangeIterator(mRange.data() + rhs * mRange.size(), mRange.size()); }
	constexpr auto  operator- (difference_type rhs) const noexcept { return RangeIterator(mRange.data() - rhs * mRange.size(), mRange.size()); }

	constexpr friend auto operator+(difference_type lhs, const RangeIterator& rhs) noexcept { return rhs + lhs; }
	constexpr friend auto operator-(difference_type lhs, const RangeIterator& rhs) noexcept { return rhs - lhs; }

	constexpr difference_type operator-(const RangeIterator& other) const noexcept { return mRange.data() - other.mRange.data(); }

	constexpr bool operator==(const RangeIterator& other) const noexcept { return mRange.data() == other.mRange.data(); }
	constexpr bool operator!=(const RangeIterator& other) const noexcept { return mRange.data() != other.mRange.data(); }
	constexpr bool operator< (const RangeIterator& other) const noexcept { return mRange.data() <  other.mRange.data(); }
	constexpr bool operator> (const RangeIterator& other) const noexcept { return mRange.data() >  other.mRange.data(); }
	constexpr bool operator<=(const RangeIterator& other) const noexcept { return mRange.data() <= other.mRange.data(); }
	constexpr bool operator>=(const RangeIterator& other) const noexcept { return mRange.data() >= other.mRange.data(); }

	constexpr auto operator[](difference_type rhs) const noexcept { return RangeProxy<T>(mRange.data() + rhs * mRange.size(), mRange.size()); }
};
#pragma endregion
