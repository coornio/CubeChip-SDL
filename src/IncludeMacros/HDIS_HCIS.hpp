/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <new>
#include <cstddef>

/*==================================================================*/

#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Winterference-size"
#endif

#ifdef __cpp_lib_hardware_interference_size
	constexpr static auto HDIS{ std::hardware_destructive_interference_size };
	constexpr static auto HCIS{ std::hardware_constructive_interference_size };
#else
	constexpr static auto HDIS{ std::size_t(64) };
	constexpr static auto HCIS{ std::size_t(64) };
#endif

#ifdef __clang__
	#pragma clang diagnostic pop
#endif
