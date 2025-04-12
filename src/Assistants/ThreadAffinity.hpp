/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <thread>
#include <algorithm>

#if defined(_WIN32)
	#define NOMINMAX
	#include <windows.h>
#elif defined(__linux__)
	#include <pthread.h>
	#include <sched.h>
	#include <unistd.h>
#elif defined(__APPLE__)
	#include <pthread.h>
	#include <sys/sysctl.h>
	#include <mach/mach.h>
	#include <mach/thread_policy.h>
#endif

namespace thread_affinity {
	/**
	 * @brief Guesstimate of amount of logical cores the system has. Defaults to 1.
	 * @return Returns core amount.
	 */
	inline unsigned get_logical_core_count() noexcept {
	#if defined(_WIN32)
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		return sysinfo.dwNumberOfProcessors;
	#elif defined(__linux__) || defined(__APPLE__)
		return std::max(1u, std::thread::hardware_concurrency()); // fallback
	#else
		return 1; // Web or unknown
	#endif
	}

	/**
	 * @brief Guesstimate of which logical processor core the current thread runs on. Defaults to 0.
	 * @return Returns processor ID number.
	 */
	inline unsigned get_current_core() {
	#if defined(_WIN32)
		return GetCurrentProcessorNumber();
	#elif defined(__linux__)
		return std::max(0, sched_getcpu());
	#else
		return 0; // MacOS or unknown
	#endif
	}

#if defined(_WIN32)
	/**
	 * @brief Sets thread affinity to desired logical cores. Will ignore invalid affinity masks safely.
	 * @param[in] affinity_mask :: Bitwise mask to control which logical cores are eligible.
	 * @param[in] thread_handle :: Thread handle pointer to (optionally) target a different thread, otherwise pick own thread.
	 * @return Boolean if successful.
	 */
	inline bool set_affinity(unsigned long long affinity_mask, void* thread_handle = nullptr) noexcept {
		void* chosen_thread{ thread_handle ? thread_handle : GetCurrentThread() };
		return SetThreadAffinityMask(static_cast<HANDLE>(chosen_thread), affinity_mask & ((1ull << get_logical_core_count()) - 1));
	}
#elif defined(__linux__)
	/**
	 * @brief Sets thread affinity to desired logical cores. Will ignore invalid affinity masks safely.
	 * @param[in] affinity_mask :: Bitwise mask to control which logical cores are eligible.
	 * @param[in] thread_handle :: Thread handle pointer to (optionally) target a different thread, otherwise pick own thread.
	 * @return Boolean if successful.
	 */
	inline bool set_affinity(unsigned long long affinity_mask, void* thread_handle = nullptr) noexcept {
		if ((affinity_mask & ((1ull << get_logical_core_count()) - 1)) == 0x0) { return false; }

		cpu_set_t cpu_aff; CPU_ZERO(&cpu_aff);

		for (auto i{ 0 }; i < 64; ++i) {
			if (affinity_mask & (1ull << i))
				{ CPU_SET(i, &cpu_aff); }
		}

		auto chosen_thread{ thread_handle ? *static_cast<pthread_t*>(thread_handle) : pthread_self() };
		return pthread_setaffinity_np(chosen_thread, sizeof(cpu_set_t), &cpu_aff) == 0;
	}
#elif defined(__APPLE__)
	/**
	 * @brief Sets thread affinity to desired tag. Tag "0" will clear an existing tag.
	 * @param[in] affinity_tag :: Tag value which controls the grouping of threads, if eligible.
	 * @param[in] thread_handle :: Thread handle pointer to (optionally) target a different thread, otherwise pick own thread.
	 * @return Boolean if successful.
	 */
	inline bool set_affinity(unsigned long long affinity_tag, void* thread_handle = nullptr) noexcept {
		auto chosen_tag{ static_cast<integer_t>(affinity_tag) };
		auto chosen_thread{ thread_handle ? *static_cast<pthread_t*>(thread_handle) : pthread_self() };

		return thread_policy_set(
			pthread_mach_thread_np(chosen_thread), THREAD_AFFINITY_POLICY,
			&chosen_tag, THREAD_AFFINITY_POLICY_COUNT) == KERN_SUCCESS;
	}
#else
	/**
	 * @brief Unsupported in the current platform. Effectively a no-op.
	 */
	inline bool set_affinity(unsigned long long, void* = nullptr) noexcept { return false; }
#endif
}
