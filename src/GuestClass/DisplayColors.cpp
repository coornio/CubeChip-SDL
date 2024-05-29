/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>

#include "DisplayColors.hpp"

DisplayColors::DisplayColors() {
	initBitColors();
	initHexColors();
}

void DisplayColors::initBitColors() {
	for (auto i{ 0 }; std::cmp_less(i, 16); ++i) {
		bit[i] = BitColors[i];
	}
}

void DisplayColors::initHexColors() {
	static constexpr auto r{ 0xFFu }, g{ 0xFFu }, b{ 0xFFu };

	for (auto i{ 0 }; std::cmp_less(i, 10); ++i) {
		const float mult{ 1.0f - 0.045f * static_cast<float>(i) };
		const float R   { r * mult * 1.03f };
		const float G   { g * mult * 1.14f };
		const float B   { b * mult * 1.21f };

		hex[i] = 0xFF000000u
			| static_cast<std::uint32_t>(std::min(std::roundf(R), 255.0f)) << 16u
			| static_cast<std::uint32_t>(std::min(std::roundf(G), 255.0f)) <<  8u
			| static_cast<std::uint32_t>(std::min(std::roundf(B), 255.0f));
	}
}

void DisplayColors::setBit332(const std::size_t idx, const std::size_t color) {
	static constexpr std::uint8_t map3b[]{ 0x00u, 0x20u, 0x40u, 0x60u, 0x80u, 0xA0u, 0xC0u, 0xFFu };
	static constexpr std::uint8_t map2b[]{ 0x00u,               0x60u,        0xA0u,        0xFFu };

	bit[idx & 0xFu] = 0xFF000000u
		| map3b[color >> 5u & 0x7u] << 16u // red
		| map3b[color >> 2u & 0x7u] <<  8u // green
		| map2b[color       & 0x3u];       // blue
}

void DisplayColors::cycleBackground() {
	bit[0] = BackColors[bgindex++ & 0x3u];
}

std::uint32_t DisplayColors::getFore8X(const std::int32_t idx) const {
	return ForeColors[idx & 0x7u];
}
