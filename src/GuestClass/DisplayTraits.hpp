/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <array>

#include "Enums.hpp"
#include "../Types.hpp"

class DisplayTraits final {
	void initBitColors();
	void initHexColors();

public:
	explicit DisplayTraits(u32* const);

	struct Traits{
		friend class DisplayTraits;

		s32 W{}, H{};
		s32 Wb{}, Hb{};
		s32 S{};

	private:
		bool _isLoresExtended{};
		bool _isManualRefresh{};
		bool _isPixelTrailing{};
		bool _isPixelBitColor{};

	public:
		s32 maskPlane{ 1 };
		s32 mask8X{ 0xFC };

		using enum BrushType;
		BrushType paintBrush{ XOR };
	} Trait;

	[[nodiscard]] bool isLoresExtended() const { return Trait._isLoresExtended; }
	[[nodiscard]] bool isManualRefresh() const { return Trait._isManualRefresh; }
	[[nodiscard]] bool isPixelTrailing() const { return Trait._isPixelTrailing; }
	[[nodiscard]] bool isPixelBitColor() const { return Trait._isPixelBitColor; }
	void isLoresExtended(bool state) { Trait._isLoresExtended = state; }
	void isManualRefresh(bool state) { Trait._isManualRefresh = state; }
	void isPixelTrailing(bool state) { Trait._isPixelTrailing = state; }
	void isPixelBitColor(bool state) { Trait._isPixelBitColor = state; }

	struct Colors {
		static constexpr u32 BitColors[]{ // 0-1 classic8, 0-15 modernXO
			0x0C1218, 0xE4DCD4, 0x8C8884, 0x403C38,
			0xD82010, 0x40D020, 0x1040D0, 0xE0C818,
			0x501010, 0x105010, 0x50B0C0, 0xF08010,
			0xE06090, 0xE0F090, 0xB050F0, 0x704020,
		};
		static constexpr u32 ForeColors[]{ // 8X foreground
			0x000000, 0xEE1111, 0x1111EE, 0xEE11EE,
			0x11EE11, 0xEEEE11, 0x11EEEE, 0xEEEEEE,
		};
		static constexpr u32 BackColors[]{ // 8X background
			0x111133, 0x111111, 0x113311, 0x331111,
		};

		u32 bit[16]{}; // pixel bit color (planes)
		u32 fade[3]{}; // pixel fade color (frames)
		u32 buzz[2]{}; // colors for buzz glow
		u32 hex[10]{}; // mega char sprite gradient
		u32 bgindex{}; // background color cycle index

		void setBit332(usz, usz);
		void setBackgroundTo(u32* const) const;
		void setBackgroundTo(u32* const, const u32) const;
		void cycleBackground(u32* const);
		u32  getFore8X(s32) const;
	} Color;

	struct Texture {
		s32 W{}, H{};

		u8    collision{ 0xFF };
		u8    rgbmod{};
		bool  rotate{};
		bool  flip_X{};
		bool  flip_Y{};
		bool  invert{};
		bool  nodraw{};
		bool  uneven{};
		float alpha{ 1.0f };

		void  setFlags(usz);
	} Tex;
};
