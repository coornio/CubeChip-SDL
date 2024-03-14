/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../Includes.hpp"

/*------------------------------------------------------------------*/
/*  interface class  FncSetInterface                                */
/*------------------------------------------------------------------*/

struct FncSetInterface {
    virtual void scrollUP(usz) = 0;
    virtual void scrollDN(usz) = 0;
    virtual void scrollLT(usz) = 0;
    virtual void scrollRT(usz) = 0;
    virtual void drawSprite(usz, usz, usz, usz) = 0;
    virtual void drawColors(usz, usz, usz, usz) = 0;

    virtual ~FncSetInterface() {};
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForGigachip                             */
/*------------------------------------------------------------------*/

class FunctionsForGigachip final : public FncSetInterface {
    VM_Guest& vm;

    struct SrcColor { float A{}, R{}, G{}, B{}; } src;
    struct DstColor { float A{}, R{}, G{}, B{}; } dst;

    float (*blendType)(float, float) {};

    u32  blendPixel(u32, u32&);
    u32  applyBlend(float (*)(float, float)) const;

    enum Trait : unsigned {
        RGB, BRG, GBR,
        RBG, GRB, BGR,
        GRAY, SEPIA,
    };

    enum Blend : unsigned {
        NORMAL,
        LIGHTEN_ONLY, SCREEN,   COLOR_DODGE, LINEAR_DODGE,
        DARKEN_ONLY,  MULTIPLY, COLOR_BURN,  LINEAR_BURN,
        AVERAGE, DIFFERENCE, NEGATION,
        OVERLAY, REFLECT,    GLOW,
        OVERWRITE,
    };

    void scrollUP(usz) override;
    void scrollDN(usz) override;
    void scrollLT(usz) override;
    void scrollRT(usz) override;
    void drawSprite(usz, usz, usz, usz) override;
    void drawColors(usz, usz, usz, usz) override {};

public:
    void chooseBlend(usz);
    FunctionsForGigachip(VM_Guest& ref) : vm(ref) {
        chooseBlend(Blend::NORMAL);
    };
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForMegachip                             */
/*------------------------------------------------------------------*/

class FunctionsForMegachip final : public FncSetInterface {
    VM_Guest& vm;
    
    struct SrcColor { float A{}, R{}, G{}, B{}; } src;
    struct DstColor { float A{}, R{}, G{}, B{}; } dst;

    float (*blendType)(float, float) {};

    u32  blendPixel(u32, u32);
    u32  applyBlend(float (*)(float, float)) const;
    void blendToDisplay(auto&, auto&);

    enum Blend : unsigned {
        NORMAL       = 0,
        LINEAR_DODGE = 4,
        MULTIPLY     = 5,
    };

    void scrollUP(usz) override;
    void scrollDN(usz) override;
    void scrollLT(usz) override;
    void scrollRT(usz) override;
    void drawSprite(usz, usz, usz, usz) override;
    void drawColors(usz, usz, usz, usz) override {};

public:
    void chooseBlend(usz);
    FunctionsForMegachip(VM_Guest& ref) : vm(ref) {
        chooseBlend(Blend::NORMAL);
    };
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForModernXO                             */
/*------------------------------------------------------------------*/

class FunctionsForModernXO final : public FncSetInterface {
    VM_Guest& vm;

    void drawByte(usz, usz, usz, usz, usz, usz);
    usz  bitBloat(usz);
    void applyBrush(u32&, usz);

    void scrollUP(usz) override;
    void scrollDN(usz) override;
    void scrollLT(usz) override;
    void scrollRT(usz) override;
    void drawSprite(usz, usz, usz, usz) override;
    void drawColors(usz, usz, usz, usz) override {};

public:
    FunctionsForModernXO(VM_Guest& ref) : vm(ref) {};
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForLegacySC                             */
/*------------------------------------------------------------------*/

class FunctionsForLegacySC final : public FncSetInterface {
    VM_Guest& vm;

    void drawByte(usz, usz, usz, usz, usz, usz);
    void drawShort(usz, usz, usz, usz, usz, usz);
    usz  bitBloat(usz);

    void scrollUP(usz) override;
    void scrollDN(usz) override;
    void scrollLT(usz) override;
    void scrollRT(usz) override;
    void drawSprite(usz, usz, usz, usz) override;
    void drawColors(usz, usz, usz, usz) override;

public:
    FunctionsForLegacySC(VM_Guest& ref) : vm(ref) {};
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForClassic8                             */
/*------------------------------------------------------------------*/

class FunctionsForClassic8 final : public FncSetInterface {
    VM_Guest& vm;

    void drawByte(usz, usz, usz, usz, usz, usz);

    void scrollUP(usz) override;
    void scrollDN(usz) override;
    void scrollLT(usz) override;
    void scrollRT(usz) override;
    void drawSprite(usz, usz, usz, usz) override;
    void drawColors(usz, usz, usz, usz) override;

public:
    FunctionsForClassic8(VM_Guest& ref) : vm(ref) {};
};
