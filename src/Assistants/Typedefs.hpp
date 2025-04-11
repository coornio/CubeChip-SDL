/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <new>
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

#define CONCAT_TOKENS_INTERNAL(x, y) x##y
#define CONCAT_TOKENS(x, y) CONCAT_TOKENS_INTERNAL(x, y)

#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Winterference-size"
#endif

#ifdef __cpp_lib_hardware_interference_size
	constexpr static auto HDIS{ std::hardware_destructive_interference_size };
	constexpr static auto HCIS{ std::hardware_constructive_interference_size };
#else
	constexpr static auto HDIS{ size_type(64) };
	constexpr static auto HCIS{ size_type(64) };
#endif

#ifdef __GNUC__
	#pragma GCC diagnostic pop
#endif

#ifdef __APPLE__
	#define EXEC_POLICY(policy)
	#include "AtomicSharedProxy.hpp"

	template <typename T>
	using AtomSharedPtr = AtomSharedProxy<T>;
#else
	#define EXEC_POLICY(policy) std::execution::policy,

	template <typename T>
	using AtomSharedPtr = Atom<std::shared_ptr<T>>;
#endif
