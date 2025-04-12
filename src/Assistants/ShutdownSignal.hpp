/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <atomic>

class ShutdownSignal {
	static inline std::atomic<bool>
		sIsRequested{};

public:
	static void isRequested(bool state) noexcept { sIsRequested.store(state, std::memory_order_release); }
	static bool isRequested()           noexcept { return sIsRequested.load(std::memory_order_relaxed); }

	static void registerHandler() noexcept;
};

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>

void ShutdownSignal::registerHandler() noexcept {
	SetConsoleCtrlHandler([](DWORD signal) -> BOOL {
		switch (signal) {
			case CTRL_C_EVENT:
			case CTRL_CLOSE_EVENT:
				isRequested(true);
				return TRUE;

			default:
				return FALSE;
		}
	}, TRUE);
}

#elif defined(__unix__) || defined(__APPLE__)
#include <csignal>

void ShutdownSignal::registerHandler() noexcept {
	std::signal(SIGINT,  [](int) { ShutdownSignal::isRequested(true); });
	std::signal(SIGTERM, [](int) { ShutdownSignal::isRequested(true); });
}

#else
	void ShutdownSignal::registerHandler() noexcept {} // no-op

#endif
