/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../Includes.hpp"
#include "../InstructionSets/Interface.hpp"

class VM_Guest {
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

    enum Resolution : u8 {
        ERROR,
        HI, // 128 x  64
        LO, //  64 x  32
        TP, //  64 x  64
        FP, //  64 x 128
        MC, // 256 x 192
    };

    enum class Interrupt : u32 {
        NONE,
        ONCE,
        STOP,
        WAIT,
        FX0A,
    };

    enum class BrushType : u32 {
        CLR,
        XOR,
        SUB,
        ADD,
    };

    class MemoryBanks {
        VM_Guest& vm;
        void (*applyViewportMask)(u32&, u32) {};
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
        void loadPalette(u32, u8);

        void clearPages(s32);
    };

    std::unique_ptr<MemoryBanks> MemoryBanksPtr{
        std::make_unique<MemoryBanks>(*this)
    };
    MemoryBanks& Mem{ *MemoryBanksPtr.get() };

    class ProgramControl {
        VM_Guest& vm;
        FncSetInterface*& fncSet;
    public:
        s32 ipf{}, boost{};
        double framerate{};

        Interrupt interrupt{};
        u32 limiter{};
        u32 opcode{};
        u32 counter{};

        u8 screenMode{};

        struct TimerData {
            u8 delay{};
            u8 sound{};
        } Timer;

        explicit ProgramControl(VM_Guest&, FncSetInterface*&);
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

    class AudioCores {
        VM_Guest& vm;
    public:
        const u32&   outFreq;
        const s16& amplitude;
        const float& volume;
        float wavePhase{};

        explicit AudioCores(VM_Guest&);
        void renderAudio(s16*, u32);

        class Classic {
            AudioCores& Audio;
        public:
            std::atomic<float> tone{};
            bool beepFx0A{};
            
            explicit Classic(AudioCores&);
            void setTone(u8, u32);
            void setTone(u8);
            void render(s16*, size_t); 
        } C8{ *this };

        class XOchip {
            AudioCores& Audio;
        public:
            std::array<u8, 16> pattern{};
            std::atomic<float> tone{};
            bool enabled{};

            explicit XOchip(AudioCores&);
            void setPitch(u8);
            void loadPattern(u32);
            void render(s16*, size_t);  
        } XO{ *this };

        class MegaChip {
            AudioCores& Audio;
        public:
            std::atomic<u32> length{};
            std::atomic<u32> start{};

            std::atomic<double> step{};
            std::atomic<double> pos{};

            bool enabled{};
            bool looping{};

            explicit MegaChip(AudioCores&);
            void reset();
            void enable(u32, u32, u32, bool);
            void render(s16*, size_t);
        } MC{ *this };
    } Audio{ *this };

    class Registers {
        VM_Guest& vm;
    public:
        std::array<u32, 16> stack{};
        std::array<u8,  16> V{};
        std::array<u8,  16> P{};
        u32 I{}; u8 SP{}, pageGuard{};

        explicit Registers(VM_Guest&);
        void routineCall(u32);
        void routineReturn();
        void protectPages();
    } Reg{ *this };

    struct BitPlaneProperties {
        s16 W{},  H{},  X{};
        s16 Wb{}, Hb{}, Xb{};
        u32 selected{ 1 };

        using enum BrushType;
        BrushType brush{ XOR };
        u32 mask{ 0x11111111 };
    } Plane;

    struct TextureTraits {
        u16 W{}, H{};
        float alpha{ 1.0f };
        u8 collision{ 0xFF };

        bool rotate{};
        bool flip_X{};
        bool flip_Y{};
        bool invert{};
        u8   rgbmod{};
        bool nodraw{};
        bool uneven{};

        void transform(u8);
    } Trait;

    struct EmulationQuirks {
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

    struct BehaviorStates {
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

    class DisplayColors {
        VM_Guest& vm;
    public:
        std::array<u32, 16> bit{}; // pixel bit color (planes)
        std::array<u32, 10> hex{}; // hex sprite gradient map

        u32 bgindex{}; // background color cycle index
        u32 megahex{}; // hex sprite color for megachip
        u32 buzzer{};  // buzzer color (visual beep)

        explicit DisplayColors(VM_Guest&);
        void setMegaHex(u32);
        void setBit332(u32, u8);
        void cycleBackground();
        u32 getFore8X(u8) const;

    private:
        u32 rgb332_888(const u8 color) {
            static constexpr std::array<u8, 8> map3bits{ 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xFF };
            static constexpr std::array<u8, 4> map2bits{ 0x00,             0x60,       0xA0,       0xFF };
            return map3bits[color >> 5 & 7] << 16 | // red
                   map3bits[color >> 2 & 7] <<  8 | // green
                   map2bits[color & 3];             // blue
        }

        static constexpr std::array<u32, 16> BitColors{ // 0-1 classic8, 0-15 modernXO
            0xFF0C141C, 0xFFE4DCD4, 0xFF403C38, 0xFF8C8884,
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
    } Color{ *this };


    // init functions
    bool setupMachine();
    bool romTypeCheck();
    bool romSizeCheck(u32, u16);
    void initPlatform();
    void loadFontData();
    void setupDisplay(u8, bool = false);
    void flushDisplay();

    // core functions
    void cycle();
    void instructionLoop();

    u8& mrw(u32 idx) { return Mem.ram[idx & Program.limiter]; }
    u8& VX()         { return Reg.V[(Program.opcode >> 8) & 0xF]; }
    u16 NNNN()       { return mrw(Program.counter) << 8 | mrw(Program.counter + 1); }
};
