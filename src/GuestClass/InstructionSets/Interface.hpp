/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <span>
#include <cstddef>
#include <cstdint>

#include "../../Types.hpp"

class MEGACORE;

/*------------------------------------------------------------------*/
/*  interface class  FncSetInterface                                */
/*------------------------------------------------------------------*/

struct FncSetInterface {
	virtual void scrollUP(s32) = 0;
	virtual void scrollDN(s32) = 0;
	virtual void scrollLT(s32) = 0;
	virtual void scrollRT(s32) = 0;

	virtual void drawSprite(s32, s32, s32) = 0;

	virtual void drawLoresColor(s32, s32, s32) = 0;

	virtual void drawHiresColor(s32, s32, s32, s32) = 0;

	virtual ~FncSetInterface() noexcept {};
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForGigachip                             */
/*------------------------------------------------------------------*/

class FunctionsForGigachip final : public FncSetInterface {
	MEGACORE& vm;

	f32(*blendAlgo)(f32, f32) noexcept {};

public:
	enum Blend {
		NORMAL,
		LIGHTEN_ONLY, SCREEN,   COLOR_DODGE, LINEAR_DODGE,
		DARKEN_ONLY,  MULTIPLY, COLOR_BURN,  LINEAR_BURN,
		AVERAGE, DIFFERENCE, NEGATION,
		OVERLAY, REFLECT,    GLOW,
		OVERWRITE,
	};

private:
	void scrollUP(s32) override;
	void scrollDN(s32) override;
	void scrollLT(s32) override;
	void scrollRT(s32) override;

	void drawSprite(s32, s32, s32) override;

	void drawLoresColor(s32, s32, s32) override {};

	void drawHiresColor(s32, s32, s32, s32) override {};

public:
	void chooseBlend(usz) noexcept;
	explicit FunctionsForGigachip(MEGACORE&) noexcept;
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForMegachip                             */
/*------------------------------------------------------------------*/

class FunctionsForMegachip final : public FncSetInterface {
	MEGACORE& vm;

	f32(*blendAlgo)(f32, f32) noexcept {};

	void blendBuffersToTexture(std::span<const u32>, std::span<const u32>);

public:
	enum Blend {
		NORMAL       = 0,
		LINEAR_DODGE = 4,
		MULTIPLY     = 5,
	};

private:
	void scrollUP(s32) override;
	void scrollDN(s32) override;
	void scrollLT(s32) override;
	void scrollRT(s32) override;

	void drawSprite(s32, s32, s32) override;

	void drawLoresColor(s32, s32, s32) override {};

	void drawHiresColor(s32, s32, s32, s32) override {};

public:
	void chooseBlend(usz) noexcept;
	explicit FunctionsForMegachip(MEGACORE&) noexcept;
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForModernXO                             */
/*------------------------------------------------------------------*/

class FunctionsForModernXO final : public FncSetInterface {
	MEGACORE& vm;

	void drawByte(s32, s32, s32, usz);

	void scrollUP(s32) override;
	void scrollDN(s32) override;
	void scrollLT(s32) override;
	void scrollRT(s32) override;

	void drawSprite(s32, s32, s32) override;

	void drawLoresColor(s32, s32, s32) override {};

	void drawHiresColor(s32, s32, s32, s32) override {};

public:
	explicit FunctionsForModernXO(MEGACORE&) noexcept;
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForLegacySC                             */
/*------------------------------------------------------------------*/

class FunctionsForLegacySC final : public FncSetInterface {
	MEGACORE& vm;

	void drawByte(s32, s32, usz, bool&);
	void drawShort(s32, s32, usz);
	usz bitBloat(usz);

	void scrollUP(s32) override;
	void scrollDN(s32) override;
	void scrollLT(s32) override;
	void scrollRT(s32) override;

	void drawSprite(s32, s32, s32) override;

	void drawLoresColor(s32, s32, s32) override;

	void drawHiresColor(s32, s32, s32, s32) override;

public:
	explicit FunctionsForLegacySC(MEGACORE&) noexcept;
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForClassic8                             */
/*------------------------------------------------------------------*/

class FunctionsForClassic8 final : public FncSetInterface {
	MEGACORE& vm;

	void drawByte(s32, s32, usz);

	void scrollUP(s32) override;
	void scrollDN(s32) override;
	void scrollLT(s32) override;
	void scrollRT(s32) override;

	void drawSprite(s32, s32, s32) override;

	void drawLoresColor(s32, s32, s32) override;

	void drawHiresColor(s32, s32, s32, s32) override;

public:
	explicit FunctionsForClassic8(MEGACORE&) noexcept;
};
