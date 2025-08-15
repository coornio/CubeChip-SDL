/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <version>

#if defined(__cpp_lib_execution) && (__cpp_lib_execution >= 201603)
	#define EXEC_POLICY(policy)
#else
	#include <execution>
	#define EXEC_POLICY(policy) std::execution::policy,
#endif
