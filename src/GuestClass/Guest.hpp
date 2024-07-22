/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <memory>
#include <string>

#include "../Types.hpp"
#include "InstructionSets/Interface.hpp" // this should be removed eventually
#include "Enums.hpp"

class HomeDirManager;
class BasicVideoSpec;
class BasicAudioSpec;


class InterfaceTest {


public:

};

class Well512;
class HexInput;
class MemoryBanks;
class SoundCores;
class DisplayTraits;

class VM_Guest final {
	FunctionsForMegachip SetGigachip{ this };
	FunctionsForMegachip SetMegachip{ this };
	FunctionsForModernXO SetModernXO{ this };
	FunctionsForLegacySC SetLegacySC{ this };
	FunctionsForClassic8 SetClassic8{ this };

	FncSetInterface* currFncSet{ &SetClassic8 };

	s32 mCyclesPerFrame{}, boost{};
	double mFramerate{};

	using enum Interrupt;
	Interrupt mInterruptType{ CLEAR };

	u8 mDelayTimer{};
	u8 mSoundTimer{};

	bool mSystemPaused{};
	u32  mTotalFrames{};
	u64  mTotalCycles{};

	std::string hexOpcode(u32) const;

	void initProgramParams(u32, s32);
	void calculateBoostCPF(s32);
	void changeFunctionSet(FncSetInterface*);

	void setInterrupt(Interrupt);
	void triggerError(std::string_view);
	void triggerOpcodeError(u32);

	void decrementTimers();
	void handleInterrupt1();
	void handleInterrupt2();

public:
	explicit VM_Guest(
		HomeDirManager&,
		BasicVideoSpec&,
		BasicAudioSpec&
	);
	~VM_Guest();

	HomeDirManager& HDM;
	BasicVideoSpec& BVS;
	BasicAudioSpec& BAS;

	std::unique_ptr<HexInput>      Input;
	std::unique_ptr<Well512>       Wrand;
	std::unique_ptr<MemoryBanks>   Mem;
	std::unique_ptr<SoundCores>    Sound;
	std::unique_ptr<DisplayTraits> Display;

	struct EmulationQuirks final {
		bool clearVF{};
		bool jmpRegX{};
		bool shiftVX{};
		bool idxRegNoInc{};
		bool idxRegMinus{};
		bool waitVblank{};
		bool waitScroll{};
		bool wrapSprite{};
	} Quirk;

	struct BehaviorStates final {
		bool chip8E_rom{};
		bool chip8X_rom{};
		bool megachip_rom{};
		bool gigachip_rom{};

		bool chip8_legacy{};
		bool schip_legacy{};
		bool hires_2paged{};
		bool hires_4paged{};
	} State;

public:
	[[nodiscard]]
	bool isSystemPaused(void) const;
	void isSystemPaused(bool);

	auto getTotalFrames() const { return mTotalFrames; }
	auto getTotalCycles() const { return mTotalCycles; }

	// init functions
	bool setupMachine();

	// core functions
	void processFrame();

private:
	void initPlatform();
	bool romTypeCheck();

	bool romCopyToMemory(usz, usz) const;
	void fontCopyToMemory() const;

	void prepDisplayArea(Resolution, bool = false);
	void renderToTexture();

	void instructionLoop();

	template <usz variant>
	void instructionDecoder();

	bool islegacyPlatform() const {
		return State.chip8E_rom
			|| State.chip8X_rom
			|| State.schip_legacy
			|| State.chip8_legacy;
	}

public:
	auto fetchCPF()       const { return mCyclesPerFrame; }
	auto fetchFramerate() const { return mFramerate; }
};
