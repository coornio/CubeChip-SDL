/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <memory>

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
class ProgramControl;
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

public:
	explicit VM_Guest(
		HomeDirManager*,
		BasicVideoSpec*,
		BasicAudioSpec*
	);
	~VM_Guest();

	HomeDirManager* const HDM;
	BasicVideoSpec* const BVS;
	BasicAudioSpec* const BAS;

	std::unique_ptr<HexInput>       Input;
	std::unique_ptr<Well512>        Wrand;
	std::unique_ptr<MemoryBanks>    Mem;
	std::unique_ptr<ProgramControl> Program;
	std::unique_ptr<SoundCores>     Sound;
	std::unique_ptr<DisplayTraits>  Display;

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

private:
	bool _isSystemPaused{};

public:
	[[nodiscard]] bool isSystemPaused() const;
	void isSystemPaused(bool);

public:
	// init functions
	bool setupMachine();

private:
	void initPlatform();
	bool romTypeCheck();

	bool romCopyToMemory(usz, usz) const;
	void fontCopyToMemory() const;

	void prepDisplayArea(Resolution, bool = false);
	void renderToTexture();

public:
	// core functions
	void cycle();

private:
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
	s32    fetchIPF()       const;
	double fetchFramerate() const;
};
