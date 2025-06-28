/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <new>
#include <cstddef>
#include <malloc.h>

template <typename T, std::size_t N = std::hardware_destructive_interference_size>
struct AlignedAlloc {
	using value_type      = T;
	using size_type       = std::size_t;
	using difference_type = std::ptrdiff_t;

	using pointer       = T*;
	using const_pointer = const T*;

	using reference       = T&;
	using const_reference = T&;

	inline AlignedAlloc() noexcept {}

	template <typename T2>
	inline AlignedAlloc(const AlignedAlloc<T2, N>&) noexcept {}

	inline ~AlignedAlloc() noexcept {}

	// == Operators.
	/**
	 * Returns false if and only if storage allocated from *this
	 *  can be deallocated from other, and vice versa.
	 *  Always returns false for stateless allocators.
	 *
	 * \param _aaOther The object against which to compare.
	 * \return Returns false.
	 **/
	bool operator!=(const AlignedAlloc<T, N>& _aaOther) const
		{ return !((*this) == _aaOther); }

	/**
	 * Returns true if and only if storage allocated from *this
	 *  can be deallocated from other, and vice versa.
	 *  Always returns true for stateless allocators.
	 *
	 * \param _aaOther The object against which to compare.
	 * \return Returns true.
	 **/
	bool operator==(const AlignedAlloc<T, N>& _aaOther) const
		{ return true; }

	// == Functions.
	/**
	 * Gets a pointer to the given reference.
	 *
	 * \param _rR The reference whose pointer is to be obtained.
	 * \return Returns a pointer to the given reference.
	 **/
	inline auto address(reference _rR)
		{ return &_rR; }

	/**
	 * Gets a constant pointer to the given reference.
	 *
	 * \param _rR The reference whose pointer is to be obtained.
	 * \return Returns a constant pointer to the given reference.
	 **/
	inline auto address(const_reference _rR) const
		{ return &_rR; }

	/**
	 * Performs an aligned allocation of _sN elements.
	 *
	 * \param _sN The number of elements to allocate.
	 * \return Returns a pointer to the allocated _sN elements or nullptr.
	 **/
	inline auto allocate(size_type _sN)
		{ return reinterpret_cast<pointer>(::_aligned_malloc(_sN * sizeof(value_type), N)); }

	/**
	 * Deallocation of the given pointer.
	 *
	 * \param _pP The pointer to deallocate.
	 **/
	inline void deallocate(pointer _pP, size_type)
		{ ::_aligned_free(_pP); }

	/**
	 * Constructs an object at the given pointer.
	 *
	 * \param _pP The address at which to construct the object.
	 * \param _vtErt The constructed value.
	 **/
	inline void construct(pointer _pP, const value_type& _vtErt)
		{ new (_pP) value_type(_vtErt); }

	/**
	 * Calls the destructor for the given value at address _pP.
	 *
	 * \param _pP The address of the item to deconstruct.
	 **/
	inline void destroy(pointer _pP)
		{ _pP->~value_type(); }

	/**
	 * Returns the maximum number of items that can fit into the vector.
	 *
	 * \return * Returns the maximum number of items that can fit into the vector.
	 **/
	inline auto max_size() const noexcept
		{ return size_type(-1) / sizeof(value_type); }

	template <typename T2>
	struct rebind {
		using other = AlignmentAllocator<T2, N>;
	};
};
