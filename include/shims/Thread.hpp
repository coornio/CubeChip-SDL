/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

/*==================================================================*/

#if defined(__has_include) && __has_include(<stop_token>) \
&& defined(__cpp_lib_jthread)
	#include <thread>

	using Thread    = std::jthread;
	using StopToken = std::stop_token;
#else
	#include "vendor/jthread/jthread.hpp"

	using Thread    = nonstd::jthread;
	using StopToken = nonstd::stop_token;
#endif
