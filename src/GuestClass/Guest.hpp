/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../Includes.hpp"
#include "../InstructionSets/Interface.hpp"
#include "HexInput.hpp"

class VM_Guest final {
    FunctionsForMegachip SetGigachip{ *this };
    FunctionsForMegachip SetMegachip{ *this };
    FunctionsForModernXO SetModernXO{ *this };
    FunctionsForLegacySC SetLegacySC{ *this };
    FunctionsForClassic8 SetClassic8{ *this };

    FncSetInterface* currFncSet{ &SetClassic8 };

public:
    explicit VM_Guest(VM_Host&);
    ~VM_Guest() = default;

    VM_Host& Host;
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

    enum class Interrupt : unsigned {
        NONE,
        ONCE,
        STOP,
        WAIT,
        FX0A,
    };

    enum class BrushType : unsigned {
        CLR,
        XOR,
        SUB,
        ADD,
    };

    class MemoryBanks final {
        VM_Guest& vm;
        void (*applyViewportMask)(u32&, usz) {};
    public:
        arr2D<u32, 256, 192> display{};
        arr2D<u32,  16, 128> bufColor8x{};
        arr2D<u32, 256, 192> bufColorMC{};
        arr2D<u8,  256, 192> bufPalette{};

        std::vector<u8> ram{};
        std::array<u32, 256> palette{};

        explicit MemoryBanks(VM_Guest&);
        void modifyViewport(BrushType);
        void changeViewportMask(BrushType);

        void flushBuffers(bool);
        void loadPalette(usz, usz);

        void clearPages(usz);
    };

private:
    std::unique_ptr<MemoryBanks> MemoryBanksPtr{
        std::make_unique<MemoryBanks>(*this)
    };
public:
    MemoryBanks& Mem{ *MemoryBanksPtr.get() };

    class ProgramControl final {
        VM_Guest& vm;
        FncSetInterface*& fncSet;
    public:
        s32 ipf{}, boost{};
        double framerate{};

        usz limiter{};
        usz screenMode{};
        u32 opcode{};
        u32 counter{};
        
        Interrupt interrupt{};

        struct TimerData {
            u8 delay{};
            u8 sound{};
        } Timer;

        explicit ProgramControl(VM_Guest&, FncSetInterface*&);
        std::string hexOpcode() const;

        void init(u32, s32);
        void setSpeed(s32);
        void setFncSet(FncSetInterface*);

        void skipInstruction();
        void jumpInstruction(u32);
        void stepInstruction(s32);
        
        void requestHalt();
        void setInterrupt(Interrupt);

        void handleTimersDec();
        void handleInterrupt();
    } Program{ *this, currFncSet };

    class AudioCores final {
        VM_Guest& vm;
    public:
        const s32& outFreq;
        const s32& volume;
        const s16& amplitude;
        float wavePhase{};
        bool  beepFx0A{};

        explicit AudioCores(VM_Guest&);
        void renderAudio(s16*, s32);

        class Classic final {
            AudioCores& Audio;
            std::atomic<float> tone{};
        public:
            explicit Classic(AudioCores&);

            void setTone(usz, usz);
            void setTone(usz);
            void render(s16*, s32);
        } C8{ *this };

        class XOchip final {
            AudioCores& Audio;
            const float rate;
            std::array<std::atomic<u8>, 16> pattern{};
            std::atomic<float> tone{};
            bool enabled{};
        public:
            explicit XOchip(AudioCores&);
            bool isOn() const;

            void setPitch(usz);
            void loadPattern(usz);
            void render(s16*, s32);
        } XO{ *this };

        class MegaChip final {
            AudioCores& Audio;
            std::atomic<usz> length{};
            std::atomic<usz> start{};
            std::atomic<double> step{};
            std::atomic<double> pos{};
            bool enabled{};
            bool looping{};
        public:
            explicit MegaChip(AudioCores&);
            bool isOn() const;

            void reset();
            void enable(usz, usz, usz, bool);
            void render(s16*, s32);
        } MC{ *this };
    } Audio{ *this };

    class Registers final {
        VM_Guest& vm;
    public:
        std::array<u32, 16> stack{};
        std::array<u8,  16> V{};
        u32 I{}, SP{}, pageGuard{};

        explicit Registers(VM_Guest&);
        void routineCall(u32);
        void routineReturn();
        void protectPages();
        bool readPermRegs(usz);
        bool writePermRegs(usz);
    } Reg{ *this };

    struct BitPlaneProperties final {
        s32 W{},  H{},  X{};
        s32 Wb{}, Hb{}, Xb{};

        u32 selected{ 1 };
        u32 mask{ 0x11111111 };

        using enum BrushType;
        BrushType brush{ XOR };
    } Plane;

    struct TextureTraits final {
        s32 W{}, H{};
        
        u8   collision{ 0xFF };
        bool rotate{};
        bool flip_X{};
        bool flip_Y{};
        bool invert{};
        u8   rgbmod{};
        bool nodraw{};
        bool uneven{};

        float alpha{ 1.0f };

        void setFlags(usz);
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

    class DisplayColors final {
        static constexpr std::array<u32, 16> BitColors{ // 0-1 classic8, 0-15 modernXO
            0xFF0C1218, 0xFFE4DCD4, 0xFF403C38, 0xFF8C8884,
            0xFFD82010, 0xFF40D020, 0xFF1040D0, 0xFFE0C818,
            0xFF501010, 0xFF105010, 0xFF50B0C0, 0xFFF08010,
            0xFFE06090, 0xFFE0F090, 0xFFB050F0, 0xFF704020,
        };
        static constexpr std::array<u32, 8> ForeColors{ // 8X foreground
            0xFF000000, 0xFFFF0000, 0xFF0000FF, 0xFFFF00FF,
            0xFF00FF00, 0xFFFFFF00, 0xFF00FFFF, 0xFFFFFFFF,
        };
        static constexpr std::array<u32, 4> BackColors{ // 8X background
            0xFF000060, 0xFF000000, 0xFF002000, 0xFF200000,
        };

    public:
        std::array<u32, 16> bit{}; // pixel bit color (planes)
        std::array<u32, 10> hex{}; // hex sprite gradient map

    private:
        u32 bgindex{}; // background color cycle index
        u32 megahex{}; // hex sprite color for megachip
        u32 buzzer{};  // buzzer color (visual beep)
        [[maybe_unused]] u32 _;

    public:
        explicit DisplayColors();
        void setMegaHex(u32);
        void setBit332(usz, usz);
        void cycleBackground();
        u32  getFore8X(const usz) const;
        u32  getBuzzer() const;
    } Color;

    // init functions
    bool setupMachine();
    bool romTypeCheck();
    bool romSizeCheck(const usz, const usz);
    void initPlatform();
    void loadFontData();
    void setupDisplay(const usz, const bool = false);
    void flushDisplay();

    // core functions
    void cycle();
    void instructionLoop();

    u8& mrw(usz idx) { return Mem.ram[idx & Program.limiter]; }
    u8& VX()         { return Reg.V[(Program.opcode >> 8) & 0xF]; }
    u16 NNNN()       { return mrw(Program.counter) << 8 | mrw(Program.counter + 1); }
};

