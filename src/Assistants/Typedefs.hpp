/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <atomic>
#include <limits>
#include <string>
#include <cstdint>
#include <cstddef>
#include <execution>

#include <filesystem>
#include <string_view>

#include <SDL3/SDL_scancode.h>

using mo = std::memory_order;

using f64 = long double;
using f32 = float;

using size_type       = std::size_t;
using difference_type = std::ptrdiff_t;

using ust = size_type;
using sst = difference_type;

using u64 = std::uint64_t;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8  = std::uint8_t;

using s64 = std::int64_t;
using s32 = std::int32_t;
using s16 = std::int16_t;
using s8  = std::int8_t;

using Str  = std::string;
using StrV = std::string_view;
using Path = std::filesystem::path;

template <typename T>
using Atom = std::atomic<T>;

using namespace std::string_literals;
using namespace std::string_view_literals;

struct Epsilon {
	constexpr static ::f32 f32{ std::numeric_limits<::f32>::epsilon() };
	constexpr static ::f64 f64{ std::numeric_limits<::f64>::epsilon() };
};

#if defined(__APPLE__) && defined(__clang__) && (__clang_major__ << 10)
	#define EXEC_POLICY(policy)
#else
	#define EXEC_POLICY(policy) std::execution::policy,
#endif

