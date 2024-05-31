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
	for (auto i{ 0 }; i < 16; ++i) {
		bit[i] = BitColors[i];
	}
}

void DisplayColors::initHexColors() {
	static constexpr auto r{ 0xFF }, g{ 0xFF }, b{ 0xFF };

	for (auto i{ 0 }; i < 10; ++i) {
		const float mult{ 1.0f - 0.045f * i };
		const float R   { r * mult * 1.03f };
		const float G   { g * mult * 1.14f };
		const float B   { b * mult * 1.21f };

		hex[i] = 0xFF000000
			| static_cast<std::uint32_t>(std::min(std::roundf(R), 255.0f)) << 16
			| static_cast<std::uint32_t>(std::min(std::roundf(G), 255.0f)) <<  8
			| static_cast<std::uint32_t>(std::min(std::roundf(B), 255.0f));
	}
}

void DisplayColors::setBit332(const std::size_t idx, const std::size_t color) {
	static constexpr std::uint8_t map3b[]{ 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xFF };
	static constexpr std::uint8_t map2b[]{ 0x00,             0x60,       0xA0,       0xFF };

	bit[idx & 0xF] = 0xFF000000
		| map3b[color >> 5 & 0x7] << 16 // red
		| map3b[color >> 2 & 0x7] <<  8 // green
		| map2b[color      & 0x3];      // blue
}

void DisplayColors::cycleBackground() {
	bit[0] = BackColors[bgindex++ & 0x3];
}

std::uint32_t DisplayColors::getFore8X(const std::int32_t idx) const {
	return ForeColors[idx & 0x7];
}
