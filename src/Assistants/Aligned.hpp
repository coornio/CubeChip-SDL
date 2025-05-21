/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <span>
#include <execution>
#include <cstdlib>
#include <cassert>
#include <memory>

#include "RangeIterator.hpp"


/*==================================================================*/

#define MAX_ALIGN 0x1000

template<typename T>
concept Allocatable = std::is_object_v<T> && !std::is_abstract_v<T>;

/**
 * @brief Free-standing heap Deleter for non-trivially destructible aligned memory.
 * @tparam T :: The type of data to destroy and deallocate.
 * @tparam N :: The alignment offset (optional). Must be a power of two.
 * @param[in] size :: The amount of elements this Deleter is managing.
 */
template <Allocatable T, std::size_t N>
class AlignedTypeDeleter {
	static_assert((N & (N - 1)) == 0,
		"N must be a power of two.");
	static_assert(N <= MAX_ALIGN,
		"Exceeded maximum allowed alignment.");
	static_assert(!std::is_trivially_destructible_v<T>,
		"Deleter expects non-trivially-destructible types.");

	std::size_t mSize{};

public:
	constexpr AlignedTypeDeleter() noexcept = default;
	constexpr AlignedTypeDeleter(std::size_t size) noexcept : mSize{ size } {}

	void operator()(T* ptr) const
		noexcept(std::is_nothrow_destructible_v<T>)
	{
		std::destroy_n(EXEC_POLICY(unseq) ptr, mSize);
		::operator delete[](ptr, std::align_val_t(N));
	}

	friend constexpr void swap(AlignedTypeDeleter& lhs, AlignedTypeDeleter& rhs) noexcept
		{ std::swap(lhs.mSize, rhs.mSize); }
};

/**
 * @brief Free-standing heap Deleter for trivially destructible aligned memory.
 * @tparam T :: The type of data to destroy and deallocate.
 * @tparam N :: The alignment offset (optional). Must be a power of two.
 */
template <Allocatable T, std::size_t N>
class AlignedLiteDeleter {
	static_assert((N& (N - 1)) == 0,
		"N must be a power of two.");
	static_assert(N <= MAX_ALIGN,
		"Exceeded maximum allowed alignment.");
	static_assert(std::is_trivially_destructible_v<T>,
		"Deleter expects trivially-destructible types.");

public:
	constexpr AlignedLiteDeleter() noexcept = default;
	constexpr AlignedLiteDeleter(std::size_t) noexcept {};

	void operator()(T* ptr) const noexcept
		{ ::operator delete[](ptr, std::align_val_t(N)); }

	friend constexpr void swap(AlignedLiteDeleter&, AlignedLiteDeleter&) noexcept {}
};

/*==================================================================*/

/**
 * @brief Free-standing unique_ptr type for aligned memory.
 * @tparam T :: The type of data to manage.
 * @tparam N :: The alignment offset (optional). Must be a power of two.
 */
template <Allocatable T, std::size_t N = HDIS>
using AlignedUnique = std::unique_ptr<T[], std::conditional_t<
	std::is_trivially_destructible_v<T>,
	AlignedLiteDeleter<T, N>, AlignedTypeDeleter<T, N>
>>;

template <Allocatable T, std::size_t N>
class AlignedMemoryBlock;

template <Allocatable T, std::size_t N>
AlignedMemoryBlock<T, N> allocate(std::size_t) noexcept;

template <Allocatable T, std::size_t N>
class AlignedMemoryBlock {
	using memory_type = AlignedUnique<T, N>;
	using self = AlignedMemoryBlock;

	memory_type mAllocated;
	std::size_t mSize{};

	[[nodiscard]] memory_type mark_constructed_and_release() noexcept {
		mSize |= (1ull << 63);
		return std::move(mAllocated);
	}

	AlignedMemoryBlock(T* ptr, std::size_t size) noexcept
		: mAllocated{ ptr, { size } }, mSize{ size }
	{}

public:
	AlignedMemoryBlock(const self&) = delete;
	AlignedMemoryBlock(self&&)      = default;
	self& operator=(const self&) = delete;
	self& operator=(self&&)      = default;

	friend self allocate<T, N>(std::size_t) noexcept;

	constexpr auto element_count()  const noexcept { return mSize & ~(1ull << 63); }
	constexpr bool is_constructed() const noexcept { return mSize >> 63 != 0ull; }
	constexpr bool has_valid_ptr()  const noexcept { return mAllocated.get() != nullptr; }

	[[nodiscard]] memory_type as_value()
		noexcept(std::is_nothrow_constructible_v<T>)
		requires(std::is_default_constructible_v<T>)
	{
		if (has_valid_ptr() && !is_constructed()) {
			std::uninitialized_value_construct_n(mAllocated.get(), element_count());
			return mark_constructed_and_release();
		}
		return memory_type{};
	}

	[[nodiscard]] memory_type as_default()
		noexcept(std::is_nothrow_default_constructible_v<T>)
		requires(std::is_default_constructible_v<T>)
	{
		if (has_valid_ptr() && !is_constructed()) {
			std::uninitialized_default_construct_n(mAllocated.get(), element_count());
			return mark_constructed_and_release();
		}
		return memory_type{};
	}

	template <typename V>
	[[nodiscard]] memory_type by_fill(V&& value)
		noexcept(std::is_nothrow_copy_constructible_v<T>)
		requires(std::is_copy_constructible_v<T> && std::is_convertible_v<V&&, T>)
	{
		if (has_valid_ptr() && !is_constructed()) {
			std::uninitialized_fill_n(mAllocated.get(), element_count(), std::forward<V>(value));
			return mark_constructed_and_release();
		}
		return memory_type{};
	}

	template <typename V>
	[[nodiscard]] memory_type by_copy(std::span<const V> from)
		noexcept(std::is_nothrow_copy_constructible_v<T>)
		requires(std::is_copy_constructible_v<T> && std::is_convertible_v<V, T>)
	{
		if (has_valid_ptr() && !is_constructed() && from.size() >= element_count()) {
			std::uninitialized_copy_n(from.begin(), element_count(), mAllocated.get());
			return mark_constructed_and_release();
		}
		return memory_type{};
	}

	template <typename V>
	[[nodiscard]] memory_type by_move(std::span<V> from)
		noexcept(std::is_nothrow_move_constructible_v<T>)
		requires(std::is_move_constructible_v<T> && std::is_convertible_v<V, T>)
	{
		if (has_valid_ptr() && !is_constructed() && from.size() >= element_count()) {
			std::uninitialized_move_n(from.begin(), element_count(), mAllocated.get());
			return mark_constructed_and_release();
		}
		return memory_type{};
	}
};

template<Allocatable T, std::size_t N = HDIS>
inline AlignedMemoryBlock<T, N> allocate(std::size_t size) noexcept {
	static_assert((N & (N - 1)) == 0u,
		"N must be a power of two.");
	static_assert(N <= MAX_ALIGN,
		"Exceeded maximum allowed alignment.");

	void* ptr{ size ? ::operator new[](size * sizeof(T), std::align_val_t(N), std::nothrow) : nullptr };
	return { static_cast<T*>(ptr), { ptr != nullptr ? size : 0 } };
}

#ifdef MAX_ALIGN
	#undef MAX_ALIGN
#endif

/*==================================================================*/

template <typename T>
	requires (std::is_default_constructible_v<T>)
class Aligned {
	static_assert(!std::is_const_v<T>,
		"T must not be const-qualified.");

public:
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

private:
	AlignedUnique<T> pData{};
	size_type mSize{};

public:
	Aligned(size_type size = 0) noexcept
		: pData{ ::allocate<T>(size).as_value() }
		, mSize{ pData ? size : 0 }
	{}

	void initialize(const value_type& value = value_type()) noexcept {
		if (!size()) { return; }
		std::fill(EXEC_POLICY(unseq)
			begin(), end(), value);
	}

	void resize(size_type new_size) noexcept {
		static_assert(std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>,
			"T must be copy or move constructible.");

		if (new_size == size()) { return; }

		Aligned other(new_size);
		if (other.size() != new_size) { return; }

		/**/ if constexpr (std::is_move_constructible_v<T>) {
			std::move(EXEC_POLICY(unseq)
				data(), data() + std::min(size(), new_size), other.data());
		}
		else if constexpr (std::is_copy_constructible_v<T>) {
			std::copy(EXEC_POLICY(unseq)
				data(), data() + std::min(size(), new_size), other.data());
		}

		*this = std::move(other);
	}

	void reallocate(size_type size) {
		pData.reset();
		pData = ::allocate<T>(size).as_value();
		mSize = pData ? size : 0;
	}

	Aligned(const Aligned&)            = delete;
	Aligned& operator=(const Aligned&) = delete;

	Aligned(Aligned&&)            noexcept = default;
	Aligned& operator=(Aligned&&) noexcept = default;

public:
	constexpr       pointer   data()        { return pData.get(); }
	constexpr       reference front()       { return data()[0]; }
	constexpr       reference back()        { return data()[size() - 1]; }

	constexpr const_pointer   data()  const { return pData.get(); }
	constexpr const_reference front() const { return data()[0]; }
	constexpr const_reference back()  const { return data()[size() - 1]; }

	constexpr size_type size()       const noexcept { return mSize; }
	constexpr size_type size_bytes() const noexcept { return size() * sizeof(value_type); }
	constexpr bool      empty()      const noexcept { return size() == 0; }
	constexpr auto      span()       const noexcept { return std::span(data(), size()); }

	constexpr auto first(size_type count) const { return RangeProxy(data(), count); }
	constexpr auto last (size_type count) const { return RangeProxy(data(), size() - count); }

	// cast underlying data to a RangeProxy (span) for a lot of added functionality
	constexpr auto operator*() const noexcept { return RangeProxy(data(), size()); }

	explicit constexpr operator bool() const noexcept { return static_cast<bool>(pData); }

public:
	constexpr reference at(size_type idx) {
		if (idx >= size()) { throw std::out_of_range("Aligned.at() index out of range"); }
		return data()[idx];
	}
	constexpr reference operator[](size_type idx) {
		assert(idx < size() && "Aligned.operator[] index out of bounds");
		return data()[idx];
	}

	constexpr const_reference at(size_type idx) const {
		if (idx >= size()) { throw std::out_of_range("Aligned.at() index out of range"); }
		return data()[idx];
	}
	constexpr const_reference operator[](size_type idx) const {
		assert(idx < size() && "Aligned.operator[] index out of bounds");
		return data()[idx];
	}

public:
	constexpr iterator begin() noexcept { return data(); }
	constexpr iterator end()   noexcept { return data() + size(); }
	constexpr reverse_iterator rbegin() noexcept { return std::make_reverse_iterator(end()); }
	constexpr reverse_iterator rend()   noexcept { return std::make_reverse_iterator(begin()); }

	constexpr const_iterator begin() const noexcept { return data(); }
	constexpr const_iterator end()   const noexcept { return data() + size(); }
	constexpr const_reverse_iterator rbegin() const noexcept { return std::make_reverse_iterator(end()); }
	constexpr const_reverse_iterator rend()   const noexcept { return std::make_reverse_iterator(begin()); }

	constexpr const_iterator cbegin() const noexcept { return begin(); }
	constexpr const_iterator cend()   const noexcept { return end(); }
	constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
	constexpr const_reverse_iterator crend()   const noexcept { return rend(); }
};
