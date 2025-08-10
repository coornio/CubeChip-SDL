/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

/*==================================================================*/

#ifdef __APPLE__
	#include <memory>
	#include <mutex>
	#include <shared_mutex>

	/*
		Semi-functional mutex-based stub for unfortunate situations where
		std::atomic<std::shared_ptr<T>> is not yet officially supported
	*/
	template <typename T>
	class AtomSharedPtr {
		using shared_ptr = std::shared_ptr<T>;

		mutable std::shared_mutex mLock;
		shared_ptr mSharedPtr;

	public:
		AtomSharedPtr()
			: mSharedPtr{ nullptr }
		{}

		explicit AtomSharedPtr(const shared_ptr& new_ptr)
			: mSharedPtr{ new_ptr }
		{}

		explicit AtomSharedPtr(shared_ptr&& new_ptr)
			: mSharedPtr{ std::move(new_ptr) }
		{}

		inline void store(const shared_ptr& new_ptr, std::memory_order = std::memory_order_seq_cst) {
			std::unique_lock lock(mLock);
			mSharedPtr = new_ptr;
		}

		inline void store(shared_ptr&& new_ptr, std::memory_order = std::memory_order_seq_cst) {
			std::unique_lock lock(mLock);
			mSharedPtr = std::move(new_ptr);
		}

		auto load(std::memory_order = std::memory_order_seq_cst) const {
			std::shared_lock lock(mLock);
			return mSharedPtr;
		}

		auto exchange(const shared_ptr& new_ptr, std::memory_order = std::memory_order_seq_cst) {
			std::unique_lock lock(mLock);
			auto old{ mSharedPtr };
			mSharedPtr = new_ptr;
			return old;
		}

		auto exchange(shared_ptr&& new_ptr, std::memory_order = std::memory_order_seq_cst) {
			std::unique_lock lock(mLock);
			auto old{ std::move(mSharedPtr) };
			mSharedPtr = std::move(new_ptr);
			return old;
		}
	};

#else
	#include <atomic>
	#include <memory>

	template <typename T>
	using AtomSharedPtr = std::atomic<std::shared_ptr<T>>;
#endif
