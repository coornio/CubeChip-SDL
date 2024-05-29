/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstddef>
#include <cstdint>

class VM_Guest;

/*------------------------------------------------------------------*/
/*  interface class  FncSetInterface                                */
/*------------------------------------------------------------------*/

struct FncSetInterface {
	virtual void scrollUP(std::int32_t) = 0;
	virtual void scrollDN(std::int32_t) = 0;
	virtual void scrollLT(std::int32_t) = 0;
	virtual void scrollRT(std::int32_t) = 0;
	virtual void drawSprite(std::int32_t, std::int32_t, std::int32_t, std::uint32_t) = 0;
	virtual void drawColors(std::int32_t, std::int32_t, std::int32_t, std::int32_t) = 0;

	virtual ~FncSetInterface() {};
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForGigachip                             */
/*------------------------------------------------------------------*/

class FunctionsForGigachip final : public FncSetInterface {
	VM_Guest* vm;

	struct SrcColor { float A{}, R{}, G{}, B{}; } src;
	struct DstColor { float A{}, R{}, G{}, B{}; } dst;

	float (*blendType)(float, float) {};

	std::uint32_t blendPixel(std::uint32_t, std::uint32_t&);
	std::uint32_t applyBlend(float (*)(float, float)) const;

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

	void scrollUP(std::int32_t) override;
	void scrollDN(std::int32_t) override;
	void scrollLT(std::int32_t) override;
	void scrollRT(std::int32_t) override;
	void drawSprite(std::int32_t, std::int32_t, std::int32_t, std::uint32_t) override;
	void drawColors(std::int32_t, std::int32_t, std::int32_t, std::int32_t)  override {};

public:
	void chooseBlend(std::size_t);
	FunctionsForGigachip(VM_Guest*);
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForMegachip                             */
/*------------------------------------------------------------------*/

class FunctionsForMegachip final : public FncSetInterface {
	VM_Guest* vm;

	struct SrcColor { float A{}, R{}, G{}, B{}; } src;
	struct DstColor { float A{}, R{}, G{}, B{}; } dst;

	float (*blendType)(float, float) {};

	std::uint32_t blendPixel(std::uint32_t, std::uint32_t);
	std::uint32_t applyBlend(float (*)(float, float)) const;

	template <typename T>
	void blendToDisplay(const T*, const T*, std::size_t);

	enum Blend {
		NORMAL       = 0,
		LINEAR_DODGE = 4,
		MULTIPLY     = 5,
	};

	void scrollUP(std::int32_t) override;
	void scrollDN(std::int32_t) override;
	void scrollLT(std::int32_t) override;
	void scrollRT(std::int32_t) override;
	void drawSprite(std::int32_t, std::int32_t, std::int32_t, std::uint32_t) override;
	void drawColors(std::int32_t, std::int32_t, std::int32_t, std::int32_t)  override {};

public:
	void chooseBlend(std::size_t);
	FunctionsForMegachip(VM_Guest*);
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForModernXO                             */
/*------------------------------------------------------------------*/

class FunctionsForModernXO final : public FncSetInterface {
	VM_Guest* vm;

	void drawByte(std::int32_t, std::int32_t, std::int32_t, std::size_t);

	void scrollUP(std::int32_t) override;
	void scrollDN(std::int32_t) override;
	void scrollLT(std::int32_t) override;
	void scrollRT(std::int32_t) override;
	void drawSprite(std::int32_t, std::int32_t, std::int32_t, std::uint32_t) override;
	void drawColors(std::int32_t, std::int32_t, std::int32_t, std::int32_t)  override {};

public:
	FunctionsForModernXO(VM_Guest*);
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForLegacySC                             */
/*------------------------------------------------------------------*/

class FunctionsForLegacySC final : public FncSetInterface {
	VM_Guest* vm;

	void drawByte(std::int32_t, std::int32_t, std::size_t);
	void drawShort(std::int32_t, std::int32_t, std::size_t);
	std::size_t bitBloat(std::size_t);

	void scrollUP(std::int32_t) override;
	void scrollDN(std::int32_t) override;
	void scrollLT(std::int32_t) override;
	void scrollRT(std::int32_t) override;
	void drawSprite(std::int32_t, std::int32_t, std::int32_t, std::uint32_t) override;
	void drawColors(std::int32_t, std::int32_t, std::int32_t, std::int32_t)  override;

public:
	FunctionsForLegacySC(VM_Guest*);
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForClassic8                             */
/*------------------------------------------------------------------*/

class FunctionsForClassic8 final : public FncSetInterface {
	VM_Guest* vm;

	void drawByte(std::int32_t, std::int32_t, std::size_t);

	void scrollUP(std::int32_t) override;
	void scrollDN(std::int32_t) override;
	void scrollLT(std::int32_t) override;
	void scrollRT(std::int32_t) override;
	void drawSprite(std::int32_t, std::int32_t, std::int32_t, std::uint32_t) override;
	void drawColors(std::int32_t, std::int32_t, std::int32_t, std::int32_t)  override;

public:
	FunctionsForClassic8(VM_Guest*);
};
