/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <new>
#include <atomic>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

#include <filesystem>
#include <string_view>


#define FMT_HEADER_ONLY
#include "../Libraries/fmt/format.h"

/*==================================================================*/

using mo = std::memory_order;

using f64 = double;
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

/*==================================================================*/

#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Winterference-size"
#endif

#ifdef __cpp_lib_hardware_interference_size
	constexpr static auto HDIS{ std::hardware_destructive_interference_size };
	constexpr static auto HCIS{ std::hardware_constructive_interference_size };
#else
	constexpr static auto HDIS{ std::size_t(64) };
	constexpr static auto HCIS{ std::size_t(64) };
#endif

#ifdef __GNUC__
	#pragma GCC diagnostic pop
#endif

/*==================================================================*/

#define CONCAT_TOKENS_INTERNAL(x, y) x##y
#define CONCAT_TOKENS(x, y) CONCAT_TOKENS_INTERNAL(x, y)

#ifdef __APPLE__
	#define EXEC_POLICY(policy)
#else
	#include <execution>
	#define EXEC_POLICY(policy) std::execution::policy,
#endif

/*==================================================================*/

#ifdef __APPLE__
	#include "AtomicSharedProxy.hpp"

	template <typename T>
	using AtomSharedPtr = AtomSharedProxy<T>;
#else
	template <typename T>
	using AtomSharedPtr = Atom<std::shared_ptr<T>>;
#endif

/*==================================================================*/

#if defined(__has_include) && __has_include(<stop_token>) \
&& defined(__cpp_lib_jthread)
	#include <thread>

	using Thread    = std::jthread;
	using StopToken = std::stop_token;
#else
	#include "../Libraries/jthread/jthread.hpp"

	using Thread    = nonstd::jthread;
	using StopToken = nonstd::stop_token;
#endif

/*==================================================================*/

#if defined(__has_include) && __has_include(<expected>) \
&& defined(__cpp_lib_expected) && (__cpp_lib_expected >= 202202L)
	#include <expected>

	template <typename T, typename E>
	using Expected = std::expected<T, E>;

	template <typename E>
	using Unexpected = std::unexpected<E>;
#else
	#include "../Libraries/tartanllama/expected.hpp"

	template <typename T, typename E>
	using Expected = tl::expected<T, E>;

	template <typename E>
	using Unexpected = tl::unexpected<E>;
#endif
