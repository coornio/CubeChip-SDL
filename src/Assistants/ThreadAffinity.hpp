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

	/**
	 * @brief Sets thread affinity to desired logical core. On MacOS, a tag is used instead to recommend
	 * affinity grouping to the scheduler, with 0 meaning no influence. Other platforms are no-ops.
	 * @param[in] id_or_tag :: Physical core ID (0-based), or affinity tag.
	 * @param[in] thread :: The jthread-type pointer to query (optional). If missing, assume self-thread.
	 * @return Boolean if successful.
	 */
	inline bool set_affinity(unsigned id_or_tag, std::jthread* thread = nullptr) {
	#if defined(_WIN32)
		if (id_or_tag >= get_logical_core_count())
			[[unlikely]] { return false; }

		return SetThreadAffinityMask(
			thread ? thread->native_handle() : GetCurrentThread(),
			1ull << id_or_tag);

	#elif defined(__linux__)
		if (id_or_tag >= get_logical_core_count())
			[[unlikely]] { return false; }

		cpu_set_t cpuset; CPU_ZERO(&cpuset);
		CPU_SET(id_or_tag, &cpuset);

		return pthread_setaffinity_np(
			thread ? static_cast<pthread_t>(thread->native_handle()) : pthread_self(),
			sizeof(cpu_set_t), &cpuset) == 0;

	#elif defined(__APPLE__)
		auto affinity_tag{ static_cast<integer_t>(id_or_tag) };

		return thread_policy_set(
			pthread_mach_thread_np(thread ? thread->native_handle() : pthread_self()),
			THREAD_AFFINITY_POLICY, &affinity_tag, THREAD_AFFINITY_POLICY_COUNT) == KERN_SUCCESS;

	#else
		(void)id_or_tag;
		(void)thread;

		return false;
	#endif
	}
}
