/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "InstructionSets/Interface.hpp" // this should be removed eventually
#include "Enums.hpp" // placeholder

class HomeDirManager;
class BasicVideoSpec;
class BasicAudioSpec;

class Well512;
class HexInput;
class ProgramControl;
class MemoryBanks;
class SoundCores;
class Registers;
class DisplayColors;

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
	std::unique_ptr<Registers>      Reg;
	std::unique_ptr<DisplayColors>  Color;

	bool _isSystemPaused{};
	bool _isDisplayReady{};

	[[nodiscard]] bool isSystemPaused() const;
	[[nodiscard]] bool isDisplayReady() const;
	VM_Guest& isSystemPaused(bool);
	VM_Guest& isDisplayReady(bool);

	struct BitPlaneProperties final {
		std::int32_t W{},  H{};
		std::int32_t Wb{}, Hb{};
		std::int32_t S{};

		std::int32_t selected{ 1 };
		std::int32_t mask8X{ 0xFC };

		using enum BrushType;
		BrushType brush{ XOR };
	} Plane;

	struct TextureTraits final {
		std::int32_t W{}, H{};

		std::uint8_t collision{ 0xFF };
		std::uint8_t rgbmod{};
		bool         rotate{};
		bool         flip_X{};
		bool         flip_Y{};
		bool         invert{};
		bool         nodraw{};
		bool         uneven{};

		float   alpha{ 1.0f };

		void setFlags(std::size_t);
	} Trait;

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
	} State;

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

	// init functions
	bool setupMachine();
	bool romTypeCheck();
	bool loadRomToRam(std::size_t, std::size_t);
	void initPlatform();
	void loadFontData();
	void setupDisplay(Resolution, bool = false);
	void renderToTexture();

	// core functions
	void cycle();
	void instructionLoop();

	template <std::size_t variant>
	void instructionDecoder();

	std::int32_t fetchIPF()       const;
	double       fetchFramerate() const;

	std::uint8_t& mrw(std::size_t);
	std::uint8_t& VX();
	std::uint32_t NNNN();
};
