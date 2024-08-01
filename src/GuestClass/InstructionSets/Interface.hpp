/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstddef>
#include <cstdint>

#include "../../Types.hpp"

class VM_Guest;

/*------------------------------------------------------------------*/
/*  interface class  FncSetInterface                                */
/*------------------------------------------------------------------*/

struct FncSetInterface {
	virtual void scrollUP(s32) = 0;
	virtual void scrollDN(s32) = 0;
	virtual void scrollLT(s32) = 0;
	virtual void scrollRT(s32) = 0;

	virtual void drawSprite(
		s32, s32, s32
	) = 0;

	virtual void drawLoresColor(
		s32, s32, s32
	) = 0;

	virtual void drawHiresColor(
		s32, s32, s32, s32
	) = 0;

	virtual ~FncSetInterface() noexcept {};
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForGigachip                             */
/*------------------------------------------------------------------*/

class FunctionsForGigachip final : public FncSetInterface {
	VM_Guest& vm;

	struct SrcColor { float A{}, R{}, G{}, B{}; } src;
	struct DstColor { float A{}, R{}, G{}, B{}; } dst;

	float (*blendType)(float, float) {};

	u32 blendPixel(u32, u32&);
	u32 applyBlend(float (*)(float, float)) const;

	enum Trait {
		RGB, BRG, GBR,
		RBG, GRB, BGR,
		GRAY, SEPIA,
	};

	enum Blend {
		NORMAL,
		LIGHTEN_ONLY, SCREEN,   COLOR_DODGE, LINEAR_DODGE,
		DARKEN_ONLY,  MULTIPLY, COLOR_BURN,  LINEAR_BURN,
		AVERAGE, DIFFERENCE, NEGATION,
		OVERLAY, REFLECT,    GLOW,
		OVERWRITE,
	};

	void scrollUP(s32) override;
	void scrollDN(s32) override;
	void scrollLT(s32) override;
	void scrollRT(s32) override;

	void drawSprite(
		s32, s32, s32
	) override;

	void drawLoresColor(
		s32, s32, s32
	) override {};

	void drawHiresColor(
		s32, s32, s32, s32
	) override {};

public:
	void chooseBlend(usz);
	explicit FunctionsForGigachip(VM_Guest&) noexcept;
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForMegachip                             */
/*------------------------------------------------------------------*/

class FunctionsForMegachip final : public FncSetInterface {
	VM_Guest& vm;

	struct SrcColor { float A{}, R{}, G{}, B{}; } src;
	struct DstColor { float A{}, R{}, G{}, B{}; } dst;

	float (*blendType)(float, float) noexcept {};

	u32 blendPixel(u32, u32) noexcept;
	u32 applyBlend(float (*)(float, float)) const noexcept;

	void blendBuffersToTexture(const u32*, const u32*);

	enum Blend {
		NORMAL       = 0,
		LINEAR_DODGE = 4,
		MULTIPLY     = 5,
	};

	void scrollUP(s32) override;
	void scrollDN(s32) override;
	void scrollLT(s32) override;
	void scrollRT(s32) override;

	void drawSprite(
		s32, s32, s32
	) override;

	void drawLoresColor(
		s32, s32, s32
	) override {};

	void drawHiresColor(
		s32, s32, s32, s32
	) override {};

public:
	void chooseBlend(usz) noexcept;
	explicit FunctionsForMegachip(VM_Guest&) noexcept;
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForModernXO                             */
/*------------------------------------------------------------------*/

class FunctionsForModernXO final : public FncSetInterface {
	VM_Guest& vm;

	void drawByte(
		s32, s32, s32, usz
	);

	void scrollUP(s32) override;
	void scrollDN(s32) override;
	void scrollLT(s32) override;
	void scrollRT(s32) override;

	void drawSprite(
		s32, s32, s32
	) override;

	void drawLoresColor(
		s32, s32, s32
	) override {};

	void drawHiresColor(
		s32, s32, s32, s32
	) override {};

public:
	explicit FunctionsForModernXO(VM_Guest&) noexcept;
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForLegacySC                             */
/*------------------------------------------------------------------*/

class FunctionsForLegacySC final : public FncSetInterface {
	VM_Guest& vm;

	void drawByte(
		s32, s32, usz, bool&
	);
	void drawShort(
		s32, s32, usz
	);
	usz bitBloat(usz);

	void scrollUP(s32) override;
	void scrollDN(s32) override;
	void scrollLT(s32) override;
	void scrollRT(s32) override;

	void drawSprite(
		s32, s32, s32
	) override;

	void drawLoresColor(
		s32, s32, s32
	) override;

	void drawHiresColor(
		s32, s32, s32, s32
	) override;

public:
	explicit FunctionsForLegacySC(VM_Guest&) noexcept;
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForClassic8                             */
/*------------------------------------------------------------------*/

class FunctionsForClassic8 final : public FncSetInterface {
	VM_Guest& vm;

	void drawByte(
		s32, s32, usz
	);

	void scrollUP(s32) override;
	void scrollDN(s32) override;
	void scrollLT(s32) override;
	void scrollRT(s32) override;

	void drawSprite(
		s32, s32, s32
	) override;

	void drawLoresColor(
		s32, s32, s32
	) override;

	void drawHiresColor(
		s32, s32, s32, s32
	) override;

public:
	explicit FunctionsForClassic8(VM_Guest&) noexcept;
};
