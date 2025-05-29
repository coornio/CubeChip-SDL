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
		if (ptr) { std::destroy_n(EXEC_POLICY(unseq) ptr, mSize); }
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
	using size_type = std::size_t;

	memory_type mAllocated;
	size_type mSize{};
	size_type mOffset{};

private:
	friend self allocate<T, N>(std::size_t) noexcept;

	AlignedMemoryBlock(T* ptr, size_type size) noexcept
		: mAllocated{ ptr, { size } }, mSize{ size }
	{}

	constexpr size_type clamp_element_construction_count(size_type count) const noexcept
		{ return count ? std::min(count, remaining_count()) : remaining_count(); }

public:
	AlignedMemoryBlock(const self&) = delete;
	AlignedMemoryBlock(self&&)      = default;
	self& operator=(const self&) = delete;
	self& operator=(self&&)      = default;

public:
	constexpr size_type element_count()   const noexcept { return mSize; }
	constexpr size_type construct_count() const noexcept { return mOffset; }
	constexpr size_type remaining_count() const noexcept { return element_count() - construct_count(); }

public:
	constexpr bool is_constructed() const noexcept { return construct_count() >= element_count(); }
	constexpr bool has_valid_ptr()  const noexcept { return mAllocated.get() != nullptr; }

	[[nodiscard]] memory_type release() noexcept
		{ return has_valid_ptr() ? std::move(mAllocated) : memory_type{}; }

	/*==================================================================*/

public:
	[[nodiscard]] self& as_value(size_type count = 0u) &
		noexcept(std::is_nothrow_constructible_v<T>)
		requires(std::is_default_constructible_v<T>)
	{
		if (has_valid_ptr() && !is_constructed()) {
			const auto safe_count{ clamp_element_construction_count(count) };
			std::uninitialized_value_construct_n(EXEC_POLICY(unseq)
				mAllocated.get() + construct_count(), safe_count);
			mOffset += safe_count;
		}
		return *this;
	}

	[[nodiscard]] self as_value(size_type count = 0u) &&
		noexcept(std::is_nothrow_constructible_v<T>)
		requires(std::is_default_constructible_v<T>)
	{
		return std::move(this->as_value(count));
	}

	[[nodiscard]] self& as_default(size_type count = 0u) &
		noexcept(std::is_nothrow_default_constructible_v<T>)
		requires(std::is_default_constructible_v<T>)
	{
		if (has_valid_ptr() && !is_constructed()) {
			const auto safe_count{ clamp_element_construction_count(count) };
			std::uninitialized_default_construct_n(EXEC_POLICY(unseq)
				mAllocated.get() + construct_count(), safe_count);
			mOffset += safe_count;
		}
		return *this;
	}

	[[nodiscard]] self as_default(size_type count = 0u) &&
		noexcept(std::is_nothrow_default_constructible_v<T>)
		requires(std::is_default_constructible_v<T>)
	{
		return std::move(this->as_default(count));
	}

	template <typename V>
	[[nodiscard]] self& by_fill(V&& value, size_type count = 0u) &
		noexcept(std::is_nothrow_copy_constructible_v<T>)
		requires(std::is_copy_constructible_v<T> && std::is_convertible_v<V&&, T>)
	{
		if (has_valid_ptr() && !is_constructed()) {
			const auto safe_count{ clamp_element_construction_count(count) };
			std::uninitialized_fill_n(EXEC_POLICY(unseq)
				mAllocated.get() + construct_count(), safe_count, std::forward<V>(value));
			mOffset += safe_count;
		}
		return *this;
	}

	template <typename V>
	[[nodiscard]] self by_fill(V&& value, size_type count = 0u) &&
		noexcept(std::is_nothrow_copy_constructible_v<T>)
		requires(std::is_copy_constructible_v<T> && std::is_convertible_v<V&&, T>)
	{
		return std::move(this->by_fill(std::forward<V>(value), count));
	}

	// * No range overlap protection for trivial T=V types in fast path.
	template <typename V>
	[[nodiscard]] self& by_copy(const V* from, size_type count = 0u) &
		noexcept(std::is_nothrow_copy_constructible_v<T>)
		requires(std::is_copy_constructible_v<T> && std::is_convertible_v<V, T>)
	{
		if (has_valid_ptr() && !is_constructed()) {
			const auto safe_count{ clamp_element_construction_count(count) };
			if constexpr (std::is_same_v<T, V> && std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>) {
				std::memcpy(mAllocated.get() + construct_count(), from, safe_count * sizeof(T));
			} else {
				std::uninitialized_copy_n(EXEC_POLICY(unseq)
					from, safe_count, mAllocated.get() + construct_count());
			}
			mOffset += safe_count;
		}
		return *this;
	}

	// * No range overlap protection for trivial T=V types in fast path.
	template <typename V>
	[[nodiscard]] self by_copy(const V* from, size_type count = 0u) &&
		noexcept(std::is_nothrow_copy_constructible_v<T>)
		requires(std::is_copy_constructible_v<T> && std::is_convertible_v<V, T>)
	{
		return std::move(this->by_copy(from, count));
	}

	template <typename V>
	[[nodiscard]] self& by_move(V* from, size_type count = 0u) &
		noexcept(std::is_nothrow_move_constructible_v<T>)
		requires(std::is_move_constructible_v<T> && std::is_convertible_v<V, T>)
	{
		if (has_valid_ptr() && !is_constructed()) {
			const auto safe_count{ clamp_element_construction_count(count) };
			if constexpr (std::is_same_v<T, V> && std::is_trivially_move_constructible_v<T> && std::is_trivially_destructible_v<T>) {
				std::memmove(mAllocated.get() + construct_count(), from, safe_count * sizeof(T));
			} else {
				std::uninitialized_move_n(EXEC_POLICY(unseq)
					from, safe_count, mAllocated.get() + construct_count());
			}
			mOffset += safe_count;
		}
		return *this;
	}

	template <typename V>
	[[nodiscard]] self by_move(V* from, size_type count = 0u) &&
		noexcept(std::is_nothrow_move_constructible_v<T>)
		requires(std::is_move_constructible_v<T> && std::is_convertible_v<V, T>)
	{
		return std::move(this->by_move(from, count));
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
