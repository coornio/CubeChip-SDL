/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>

template <typename T>
class AtomSharedProxy {
	using self = AtomSharedProxy;
	using value_type = std::shared_ptr<T>;

	mutable std::shared_mutex mLock;
	value_type mSharedPtr;

public:
	AtomSharedProxy()
		: mSharedPtr{ std::make_shared<T>() }
	{}

	explicit AtomSharedProxy(const value_type& new_ptr)
		: mSharedPtr{ new_ptr }
	{}

	explicit AtomSharedProxy(value_type&& new_ptr)
		: mSharedPtr{ std::move(new_ptr) }
	{}

	inline void store(const value_type& new_ptr, std::memory_order = std::memory_order::memory_order_seq_cst) {
		std::unique_lock lock(mLock);
		mSharedPtr = new_ptr;
	}

	inline void store(value_type&& new_ptr, std::memory_order = std::memory_order::memory_order_seq_cst) {
		std::unique_lock lock(mLock);
		mSharedPtr = std::move(new_ptr);
	}

	auto load(std::memory_order = std::memory_order::memory_order_seq_cst) const {
		std::shared_lock lock(mLock);
		return mSharedPtr;
	}

	auto exchange(const value_type& new_ptr, std::memory_order) {
		std::unique_lock lock(mLock);
		value_type old = mSharedPtr;
		mSharedPtr = new_ptr;
		return old;
	}

	auto exchange(value_type&& new_ptr, std::memory_order = std::memory_order::memory_order_seq_cst) {
		std::unique_lock lock(mLock);
		value_type old = std::move(mSharedPtr);
		mSharedPtr = std::move(new_ptr);
		return old;
	}
};
