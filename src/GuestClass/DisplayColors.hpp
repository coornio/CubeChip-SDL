/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class DisplayColors final {
	static constexpr std::uint32_t BitColors[]{ // 0-1 classic8, 0-15 modernXO
		0xFF0C1218, 0xFFE4DCD4, 0xFF8C8884, 0xFF403C38,
		0xFFD82010, 0xFF40D020, 0xFF1040D0, 0xFFE0C818,
		0xFF501010, 0xFF105010, 0xFF50B0C0, 0xFFF08010,
		0xFFE06090, 0xFFE0F090, 0xFFB050F0, 0xFF704020,
	};
	static constexpr std::uint32_t ForeColors[]{ // 8X foreground
		0xFF000000, 0xFFEE1111, 0xFF1111EE, 0xFFEE11EE,
		0xFF11EE11, 0xFFEEEE11, 0xFF11EEEE, 0xFFEEEEEE,
	};
	static constexpr std::uint32_t BackColors[]{ // 8X background
		0xFF111133, 0xFF111111, 0xFF113311, 0xFF331111,
	};

public:
	std::uint32_t bit[16]{}; // pixel bit color (planes)
	std::uint32_t buzz[2]{}; // colors for buzz glow
	std::uint32_t hex[10]{}; // mega char sprite gradient

private:
	std::uint32_t bgindex{}; // background color cycle index

private:
	void initBitColors();
	void initHexColors();

public:
	DisplayColors();

	void          setBit332(std::size_t, std::size_t);
	void          cycleBackground();
	std::uint32_t getFore8X(std::int32_t) const;
};
