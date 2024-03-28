/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "../HostClass/HomeDirManager.hpp"
#include "../HostClass/BasicVideoSpec.hpp"
#include "../HostClass/BasicAudioSpec.hpp"

#include "../Assistants/Well512.hpp"

#include "InstructionSets/Interface.hpp"
#include "HexInput.hpp"
#include "ProgramControl.hpp"
#include "MemoryBanks.hpp"
#include "SoundCores.hpp"
#include "Registers.hpp"
#include "DisplayColors.hpp"

class VM_Guest final {
    FunctionsForMegachip SetGigachip{ *this };
    FunctionsForMegachip SetMegachip{ *this };
    FunctionsForModernXO SetModernXO{ *this };
    FunctionsForLegacySC SetLegacySC{ *this };
    FunctionsForClassic8 SetClassic8{ *this };

    FncSetInterface* currFncSet{ &SetClassic8 };

public:
    explicit VM_Guest(
        HomeDirManager&,
        BasicVideoSpec&,
        BasicAudioSpec&
    );
    ~VM_Guest() = default;

    HomeDirManager& File;
    BasicVideoSpec& Video;
    BasicAudioSpec& Audio;

    HexInput Input;
    Well512  Wrand;

    enum Resolution : unsigned {
        ERROR,
        HI, // 128 x  64
        LO, //  64 x  32
        TP, //  64 x  64
        FP, //  64 x 128
        MC, // 256 x 192
    };

private:
    std::unique_ptr<MemoryBanks> MemoryBanksPtr{
        std::make_unique<MemoryBanks>(*this)
    };
public:
    MemoryBanks&   Mem     { *MemoryBanksPtr.get() };
    ProgramControl Program { *this, currFncSet };
    SoundCores     Sound   { *this, Audio };
    Registers      Reg     { *this };
    DisplayColors  Color;

    struct BitPlaneProperties final {
        int32_t W{},  H{},  X{};
        int32_t Wb{}, Hb{}, Xb{};

        uint32_t selected{ 1 };
        uint32_t mask{ 0x11111111 };

        using enum BrushType;
        BrushType brush{ XOR };
    } Plane;

    struct TextureTraits final {
        int32_t W{}, H{};
        
        uint8_t collision{ 0xFF };
        bool    rotate{};
        bool    flip_X{};
        bool    flip_Y{};
        bool    invert{};
        uint8_t rgbmod{};
        bool    nodraw{};
        bool    uneven{};

        float   alpha{ 1.0f };

        void setFlags(std::size_t);
    } Trait;

    struct EmulationQuirks final {
        bool clearVF{};
        bool jmpRegX{};
        bool shiftVX{};
        bool idxRegNoInc{};
        bool idxRegMinus{};
        bool waitVblank{};
        bool waitScroll{};
        bool wrapSprite{};
        bool accuCycles{};
    } Quirk;

    struct BehaviorStates final {
        bool chip8E_rom{};
        bool chip8X_rom{};
        bool chip8X_hires{};
        bool chip_classic{};
        bool xochip_color{};
        bool chip8_legacy{};
        bool schip_legacy{};
        bool hires_2paged{};
        bool hires_4paged{};
        bool megachip_rom{};
        bool gigachip_rom{};
        bool mega_enabled{};

        bool push_display{};
    } State;

    // init functions
    bool setupMachine();
    bool romTypeCheck();
    bool romSizeCheck(const std::size_t, const std::size_t);
    void initPlatform();
    void loadFontData();
    void setupDisplay(const std::size_t, const bool = false);
    void flushDisplay();

    // core functions
    void cycle();
    void instructionLoop();

    uint8_t& mrw(std::size_t idx) { return Mem.ram[idx & Program.limiter]; }
    uint8_t& VX()                 { return Reg.V[(Program.opcode >> 8) & 0xF]; }
    uint16_t NNNN()               { return mrw(Program.counter) << 8 | mrw(Program.counter + 1); }
};

