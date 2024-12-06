/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <limits>
#include <string>
#include <cstdint>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <string_view>

#include <SDL3/SDL_scancode.h>

using f64 = long double;
using f32 = float;

using usz = std::size_t;
using u64 = std::uint64_t;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8  = std::uint8_t;

using ssz = std::ptrdiff_t;
using s64 = std::int64_t;
using s32 = std::int32_t;
using s16 = std::int16_t;
using s8  = std::int8_t;

using Str  = std::string;
using StrV = std::string_view;
using Path = std::filesystem::path;

using namespace std::string_literals;
using namespace std::string_view_literals;

struct Epsilon {
	constexpr static ::f32 f32{ std::numeric_limits<::f32>::epsilon() };
	constexpr static ::f64 f64{ std::numeric_limits<::f64>::epsilon() };
};

inline constexpr u32 KiB{ 1024 };
inline constexpr u32 MiB{ 1024 * KiB };
inline constexpr u32 GiB{ 1024 * MiB };

inline constexpr u32 CalcBytes(const u32 value, const u32 unit) noexcept {
	return value * unit;
}

template <typename T, typename E>
using Expected = std::expected<T, E>;

// factory for Expected<T, E> type, <E> should be able to override as a boolean.
template<typename T, typename E>
Expected<T, E> makeExpected(T&& value, E&& error) {
	if (error) { return std::unexpected(error); } else { return (value); }
}
