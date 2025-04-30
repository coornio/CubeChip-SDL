/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstdint>

/*==================================================================*/

struct alignas(4) RGBA {
	using u8  = std::uint8_t;
	using u32 = std::uint32_t;

	u8 R{}, G{}, B{}, A{};

	constexpr RGBA() noexcept {}
	constexpr RGBA(u32 color) noexcept
		: R{ static_cast<u8>(color >> 24) }
		, G{ static_cast<u8>(color >> 16) }
		, B{ static_cast<u8>(color >>  8) }
		, A{ static_cast<u8>(color >>  0) }
	{}
	constexpr RGBA(u8 R, u8 G, u8 B, u8 A = 0xFF) noexcept
		: R{ R }, G{ G }, B{ B }, A{ A }
	{}

	constexpr u32 RGB_()     const noexcept { return R << 24 | G << 16 | B << 8 | 0; }
	constexpr u32 RBG_()     const noexcept { return R << 24 | B << 16 | G << 8 | 0; }
	constexpr u32 GRB_()     const noexcept { return G << 24 | R << 16 | B << 8 | 0; }
	constexpr u32 GBR_()     const noexcept { return G << 24 | B << 16 | R << 8 | 0; }
	constexpr u32 BRG_()     const noexcept { return B << 24 | R << 16 | G << 8 | 0; }
	constexpr u32 BGR_()     const noexcept { return B << 24 | G << 16 | R << 8 | 0; }

	constexpr u32 RBGA()     const noexcept { return R << 24 | B << 16 | G << 8 | A; }
	constexpr u32 GRBA()     const noexcept { return G << 24 | R << 16 | B << 8 | A; }
	constexpr u32 GBRA()     const noexcept { return G << 24 | B << 16 | R << 8 | A; }
	constexpr u32 BRGA()     const noexcept { return B << 24 | R << 16 | G << 8 | A; }
	constexpr u32 BGRA()     const noexcept { return B << 24 | G << 16 | R << 8 | A; }
	constexpr operator u32() const noexcept { return R << 24 | G << 16 | B << 8 | A; }
};
