#pragma once

#include "../Includes.hpp"

/*------------------------------------------------------------------*/
/*  interface class  FncSetInterface                                */
/*------------------------------------------------------------------*/

struct FncSetInterface {
    virtual void scrollUP(s32) = 0;
    virtual void scrollDN(s32) = 0;
    virtual void scrollLT(s32) = 0;
    virtual void scrollRT(s32) = 0;
    virtual void drawSprite(u8, u8, s32, u32) = 0;
    virtual void drawColors(u8, u8, u8, s32)  = 0;

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

    enum Trait : u8 {
        RGB, BRG, GBR,
        RBG, GRB, BGR,
        GRAY, SEPIA,
    };

    enum Blend : u8 {
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
    void drawSprite(u8, u8, s32, u32) override;
    void drawColors(u8, u8, u8, s32)  override {};

public:
    FunctionsForGigachip(VM_Guest& ref) : vm(ref) {
        chooseBlend(Blend::NORMAL);
    };
    void chooseBlend(s32);
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

    enum Blend : u8 {
        NORMAL       = 0,
        LINEAR_DODGE = 4,
        MULTIPLY     = 5,
    };

    void scrollUP(s32) override;
    void scrollDN(s32) override;
    void scrollLT(s32) override;
    void scrollRT(s32) override;
    void drawSprite(u8, u8, s32, u32) override;
    void drawColors(u8, u8, u8, s32)  override {};

public:
    FunctionsForMegachip(VM_Guest& ref) : vm(ref) {
        chooseBlend(Blend::NORMAL);
    };
    void chooseBlend(s32);
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForModernXO                             */
/*------------------------------------------------------------------*/

class FunctionsForModernXO final : public FncSetInterface {
    VM_Guest& vm;

    void drawByte(s32, s32, s32, s32, s32, u32);
    u32  bitBloat(u32);
    void applyBrush(u32&, u32);

    void scrollUP(s32) override;
    void scrollDN(s32) override;
    void scrollLT(s32) override;
    void scrollRT(s32) override;
    void drawSprite(u8, u8, s32, u32) override;
    void drawColors(u8, u8, u8, s32)  override {};

public:
    FunctionsForModernXO(VM_Guest& ref) : vm(ref) {};
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForLegacySC                             */
/*------------------------------------------------------------------*/

class FunctionsForLegacySC final : public FncSetInterface {
    VM_Guest& vm;

    void drawByte(s32, s32, s32, s32, s32, u8);
    void drawShort(s32, s32, s32, s32, s32, u8);
    u16  bitBloat(u16);

    void scrollUP(s32) override;
    void scrollDN(s32) override;
    void scrollLT(s32) override;
    void scrollRT(s32) override;
    void drawSprite(u8, u8, s32, u32) override;
    void drawColors(u8, u8, u8, s32)  override;

public:
    FunctionsForLegacySC(VM_Guest& ref) : vm(ref) {};
};

/*------------------------------------------------------------------*/
/*  derived class  FunctionsForClassic8                             */
/*------------------------------------------------------------------*/

class FunctionsForClassic8 final : public FncSetInterface {
    VM_Guest& vm;

    void drawByte(s32, s32, s32, s32, s32, u8);

    void scrollUP(s32) override;
    void scrollDN(s32) override;
    void scrollLT(s32) override;
    void scrollRT(s32) override;
    void drawSprite(u8, u8, s32, u32) override;
    void drawColors(u8, u8, u8, s32)  override;

public:
    FunctionsForClassic8(VM_Guest& ref) : vm(ref) {};
};
