/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "ShutdownSignal.hpp"

/*==================================================================*/

#if defined(_WIN32)
	#define NOMINMAX
	#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
	#include <csignal>
#endif

void ShutdownSignal::registerHandler() noexcept {
#if defined(_WIN32)
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
#elif defined(__unix__) || defined(__APPLE__)
	std::signal(SIGINT,  [](int) { ShutdownSignal::isRequested(true); });
	std::signal(SIGTERM, [](int) { ShutdownSignal::isRequested(true); });
#endif
}
