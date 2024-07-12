/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>

#include "../HostClass/BasicVideoSpec.hpp"

#include "DisplayTraits.hpp"

DisplayTraits::DisplayTraits(BasicVideoSpec* BVS) {
	initBitColors();
	initHexColors();

	BVS->setBackgroundColor(Color.bit[0]);
}

void DisplayTraits::initBitColors() {
	for (auto i{ 0 }; i < 16; ++i) {
		Color.bit[i] = Color.BitColors[i];
	}
	Color.buzz[0] = Color.bit[0];
	Color.buzz[1] = Color.bit[1];
}

void DisplayTraits::initHexColors() {
	static constexpr auto r{ 0xFF }, g{ 0xFF }, b{ 0xFF };

	for (auto i{ 0 }; i < 10; ++i) {
		const float mult{ 1.0f - 0.045f * i };
		const float R   { r * mult * 1.03f };
		const float G   { g * mult * 1.14f };
		const float B   { b * mult * 1.21f };

		Color.hex[i] = 0xFF000000
			| static_cast<u32>(std::min(std::roundf(R), 255.0f)) << 16
			| static_cast<u32>(std::min(std::roundf(G), 255.0f)) <<  8
			| static_cast<u32>(std::min(std::roundf(B), 255.0f));
	}
}

void DisplayTraits::Colors::setBit332(const usz idx, const usz color) {
	static constexpr u8 map3b[]{ 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xFF };
	static constexpr u8 map2b[]{ 0x00,             0x60,       0xA0,       0xFF };

	bit[idx & 0xF] = map3b[color >> 5 & 0x7] << 16 // red
				   | map3b[color >> 2 & 0x7] <<  8 // green
				   | map2b[color      & 0x3];      // blue
}

void DisplayTraits::Colors::cycleBackground(BasicVideoSpec* BVS) {
	BVS->setBackgroundColor(BackColors[bgindex++ & 0x3]);
}

u32 DisplayTraits::Colors::getFore8X(const s32 idx) const {
	return ForeColors[idx & 0x7];
}

void DisplayTraits::Texture::setFlags(const usz bits) {
	rotate = bits >> 0 & 0x1; // false: as-is | true: 90° clockwise
	flip_X = bits >> 1 & 0x1; // flip on the X axis (rotation agnostic)
	flip_Y = bits >> 2 & 0x1; // flip on the Y axis (rotation agnostic)
	invert = bits >> 3 & 0x1; // invert RGB channels
	rgbmod = bits >> 4 & 0x7; // RGB channel swaps | sepia/grayscale
	nodraw = bits >> 7 * 0x1; // disable drawing, palette index only
	uneven = rotate && (W != H);
}
