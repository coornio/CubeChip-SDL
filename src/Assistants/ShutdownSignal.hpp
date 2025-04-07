/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <atomic>

namespace ShutdownSignal {
	namespace {
		static std::atomic<bool>
			sIsRequested{};
	}

	inline void isRequested(bool state) noexcept { sIsRequested.store(state); }
	inline bool isRequested()           noexcept { return sIsRequested.load(); }
}

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>

namespace ShutdownSignal {
	inline void registerHandler() noexcept {
		SetConsoleCtrlHandler([](DWORD signal) -> BOOL {
			if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
				isRequested(true);
				return TRUE;
			} else {
				return FALSE;
			}
		}, TRUE);
	}
}

#elif defined(__unix__) || defined(__APPLE__)
#include <csignal>

namespace ShutdownSignal {
	inline void registerHandler() noexcept {
		std::signal(SIGINT,  [](int) { ShutdownSignal::isRequested(true); });
		std::signal(SIGTERM, [](int) { ShutdownSignal::isRequested(true); });
	}
}

#else
namespace ShutdownSignal {
	inline void registerHandler() noexcept {} // no-op
}

#endif
