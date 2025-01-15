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

struct alignas(4) RGBA {
	u8 R{}, G{}, B{}, A{};

	constexpr RGBA() noexcept {}
	constexpr RGBA(const u32 color) noexcept
		: R{ color >> 24 & 0xFF }
		, G{ color >> 16 & 0xFF }
		, B{ color >>  8 & 0xFF }
		, A{ color >>  0 & 0xFF }
	{}
	constexpr RGBA(const u8 R, const u8 G, const u8 B, const u8 A = 0xFF) noexcept
		: R{ R }, G{ G }, B{ B }, A{ A }
	{}

	constexpr u32 RGB_()     const noexcept { return R << 24 | G << 16 | B << 8 | 0; }
	constexpr u32 RBG_()     const noexcept { return R << 24 | B << 16 | G << 8 | 0; }
	constexpr u32 GRB_()     const noexcept { return G << 24 | R << 16 | B << 8 | 0; }
	constexpr u32 GBR_()     const noexcept { return G << 24 | B << 16 | R << 8 | 0; }
	constexpr u32 BRG_()     const noexcept { return B << 24 | R << 16 | G << 8 | 0; }
	constexpr u32 BGR_()     const noexcept { return B << 24 | G << 16 | R << 8 | 0; }

	constexpr u32 RBGA()     const noexcept { return R << 24 | B << 16 | G << 8 | 0; }
	constexpr u32 GRBA()     const noexcept { return G << 24 | R << 16 | B << 8 | 0; }
	constexpr u32 GBRA()     const noexcept { return G << 24 | B << 16 | R << 8 | 0; }
	constexpr u32 BRGA()     const noexcept { return B << 24 | R << 16 | G << 8 | 0; }
	constexpr u32 BGRA()     const noexcept { return B << 24 | G << 16 | R << 8 | 0; }
	constexpr operator u32() const noexcept { return R << 24 | G << 16 | B << 8 | A; }
};

inline constexpr u8 IntColorMult(const u8 color1, const u8 color2) noexcept {
	return ((color1 * (color2 | color2 << 8)) + 0x8080) >> 16;
}

template <typename T, typename E>
using Expected = std::expected<T, E>;

// factory for Expected<T, E> type, <E> should be able to override as a boolean.
template<typename T, typename E>
Expected<T, E> makeExpected(T&& value, E&& error) {
	if (error) { return std::unexpected(error); } else { return (value); }
}
