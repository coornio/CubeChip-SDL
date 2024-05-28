/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>

#include "DisplayColors.hpp"

DisplayColors::DisplayColors()
	: bit{ BitColors }
{
	setMegaHex(0xFFFFFFFF);
}

void DisplayColors::setMegaHex(const uint32_t color) {
	megahex = color;

	for (auto i{ 0 }; auto& byte : hex) {
		const float mult{ 1.0f - 0.045f * i++ };
		const float R   { (color >> 16 & 0xFF) * mult * 1.03f };
		const float G   { (color >>  8 & 0xFF) * mult * 1.14f };
		const float B   { (color       & 0xFF) * mult * 1.21f };

		byte = 0xFF000000
			| static_cast<uint32_t>(std::min(std::roundf(R), 255.0f)) << 16
			| static_cast<uint32_t>(std::min(std::roundf(G), 255.0f)) <<  8
			| static_cast<uint32_t>(std::min(std::roundf(B), 255.0f));
	}
}

void DisplayColors::setBit332(const std::size_t idx, const std::size_t color) {
	static constexpr std::array<uint8_t, 8> map3b{ { 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xFF } };
	static constexpr std::array<uint8_t, 4> map2b{ { 0x00,             0x60,       0xA0,       0xFF } };

	bit[idx & 0xF] = 0xFF000000
		| map3b[color >> 5 & 7] << 16 // red
		| map3b[color >> 2 & 7] <<  8 // green
		| map2b[color      & 3];      // blue
}

void DisplayColors::cycleBackground() {
	bit[0] = BackColors[bgindex++ & 0x3];
}

uint32_t DisplayColors::getFore8X(const std::size_t idx) const {
	return ForeColors[idx & 0x7];
}
