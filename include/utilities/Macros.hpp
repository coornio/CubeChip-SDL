/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

/*==================================================================*/

#if defined(__clang__)
	#define SUGGEST_VECTORIZABLE_LOOP _Pragma("clang loop vectorize(enable)")
#elif defined(__GNUC__)
	#define SUGGEST_VECTORIZABLE_LOOP _Pragma("GCC ivdep")
#elif defined(_MSC_VER)
	#define SUGGEST_VECTORIZABLE_LOOP _Pragma("loop(ivdep)")
#else
	#define SUGGEST_VECTORIZABLE_LOOP
#endif

/*==================================================================*/

#define CONCAT_TOKENS_INTERNAL(x, y) x##y
#define CONCAT_TOKENS(x, y) CONCAT_TOKENS_INTERNAL(x, y)
