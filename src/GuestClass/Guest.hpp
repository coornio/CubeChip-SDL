/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <memory>
#include <string>
#include <array>
#include <vector>

#include "../Assistants/Well512.hpp"
#include "../Assistants/Map2D.hpp"
#include "../Types.hpp"

#include "InstructionSets/Interface.hpp" // this should be removed eventually
#include "HexInput.hpp"
#include "Enums.hpp"

class HomeDirManager;
class BasicVideoSpec;
class BasicAudioSpec;

class InterfaceTest {

public:

};

class DisplayTraits;

class VM_Guest final {
	friend class FunctionsForGigachip;
	friend class FunctionsForMegachip;
	friend class FunctionsForModernXO;
	friend class FunctionsForLegacySC;
	friend class FunctionsForClassic8;

	FunctionsForGigachip SetGigachip{ *this };
	FunctionsForMegachip SetMegachip{ *this };
	FunctionsForModernXO SetModernXO{ *this };
	FunctionsForLegacySC SetLegacySC{ *this };
	FunctionsForClassic8 SetClassic8{ *this };

	FncSetInterface* currFncSet{ &SetClassic8 };

	HomeDirManager& HDM;
	BasicVideoSpec& BVS;
	BasicAudioSpec& BAS;

	HexInput Input;
	Well512  Wrand;

/*==================================================================*/
	#pragma region AUDIO GENERATION
/*==================================================================*/

	bool mBuzzLight{};
	bool mAudioIsXO{};
	bool mAudioIsMC{};
	f32  mWavePhase{};

	struct Audio_C8 final {
		f32  mTone{};
	} C8;
	struct Audio_XO final {
		explicit Audio_XO(s32);

		f32  mStep{};
		f32  mTone{};
		u8   mData[16]{};
	} XO;
	struct Audio_MC final {
		u32  mMemPoint{};
		s32  mTrackLen{};
		f64  mStepping{};
		f64  mTrackPos{};
	} MC;

	void setAudioTone_C8();
	void setAudioTone_8X(u8);

	void setAudioTone_XO(u8);
	void fetchPattern_XO();

	void resetAudioTrack();
	void startAudioTrack(bool);

	void renderAudio_C8(std::span<s16>, s16);
	void renderAudio_XO(std::span<s16>, s16);
	void renderAudio_MC(std::span<s16>, s16);

	void renderAudioData();

/*==================================================================*/
	#pragma endregion
/*==================================================================*/

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

	// Basic VM variables
	u64  mTotalCycles{};
	u32  mTotalFrames{};

	s32  mCyclesPerFrame{};
	s32  boost{};
	f32  mFramerate{};

	Interrupt mInterruptType{};

	bool mSystemPaused{};
	/*
	bool _fontDraw{};

	[[nodiscard]]
	bool fontDraw() const { return _fontDraw; }
	void fontDraw(bool state) { _fontDraw = state; }

	[[maybe_unused]] std::array<u8,  80> fontS{};
	[[maybe_unused]] std::array<u8, 160> fontL{};
	[[maybe_unused]] std::array<u8, 160> fontM{};
	*/

	// Platform variables
	u8  mDelayTimer{};
	u8  mSoundTimer{};

	u32 mInstruction{};
	u32 mProgCounter{};

	std::vector<u8>
		 mMemoryBank{};

	u32  mStackBank[16]{};
	u8   mRegisterV[16]{};
	
	//u32* mStackTop{ mStackBank };
	u32  mStackTop{};
	u32  mRegisterI{};
	s32  pageGuard{};

	// Platform video buffers
	std::vector<u32> megaPalette;

	Map2D<u32> foregroundBuffer;
	Map2D<u32> backgroundBuffer;
	Map2D<u8>  collisionPalette;

	Map2D<u8>  displayBuffer[4];
	Map2D<u32> color8xBuffer;

	bool in_range(const usz pos) const noexcept { return pos < mMemoryBank.size(); }

	void setMemorySize(const usz val) { mMemoryBank.resize(val); }
	auto getMemorySpan()              { return std::span(mMemoryBank); }
	auto peekStackHead() const        { return mStackTop; }




	// Write memory at given index using given value
	void writeMemory(const usz value, const usz pos) {
		//mMemoryBank[pos & mMemoryBank.size() - 1] = static_cast<u8>(value);
		if (in_range(pos)) { mMemoryBank[pos] = static_cast<u8>(value); }
	}
	// Write memory at saved index using given value
	void writeMemoryI(const usz value, const usz pos) {
		//mMemoryBank[mRegisterI + pos & mMemoryBank.size() - 1] = static_cast<u8>(value);
		if (in_range(mRegisterI + pos)) { mMemoryBank[mRegisterI + pos] = static_cast<u8>(value); }
	}
	// Write memory at saved index using given value
	void writeMemoryI(const usz value) {
		//mMemoryBank[mRegisterI & mMemoryBank.size() - 1] = static_cast<u8>(value);
		if (in_range(mRegisterI)) { mMemoryBank[mRegisterI] = static_cast<u8>(value); }
	}

	// Read memory at given index
	auto readMemory(const usz pos) const -> u8 {
		return (in_range(pos)) ? mMemoryBank[pos] : 0;
		//return mMemoryBank[pos & mMemoryBank.size() - 1];
	}
	// Read memory at saved index
	auto readMemoryI(const usz pos) const -> u8 {
		return (in_range(mRegisterI + pos)) ? mMemoryBank[mRegisterI + pos] : 0;
		//return mMemoryBank[mRegisterI + pos & mMemoryBank.size() - 1];
	}
	// Read memory at saved index
	auto readMemoryI() const -> u8 {
		return (in_range(mRegisterI)) ? mMemoryBank[mRegisterI] : 0;
		//return mMemoryBank[mRegisterI & mMemoryBank.size() - 1];
	}

	bool stateRunning() const { return (
		mInterruptType != Interrupt::FINAL &&
		mInterruptType != Interrupt::ERROR
	); }
	bool stateStopped() const { return (
		mInterruptType == Interrupt::FINAL ||
		mInterruptType == Interrupt::ERROR
	); }
	bool stateWaitKey() const { return (
		mInterruptType == Interrupt::INPUT
	); }
	bool stateWaiting() const { return (
		mInterruptType == Interrupt::SOUND ||
		mInterruptType == Interrupt::DELAY ||
		mInterruptType == Interrupt::INPUT
	); }


private:
	bool routineCall(u32);
	bool routineReturn();

	void protectPages();

	bool readPermRegs(usz);
	bool writePermRegs(usz);

	void skipInstruction(); // to be deprecated
	void skipInstruction_C8();
	void skipInstruction_MC();
	void skipInstruction_XO();
	void skipInstruction_HW();
	bool jumpInstruction_C8(u32);
	bool jumpInstruction_8E(s32);

	void modifyDisplay_C8();
	void modifyDisplay_XO();
	void modifyDisplay_HW();

	void flushBuffers(FlushType);
	void loadMegaPalette(s32);

	void clearPages();

	auto  NNNN() const { return readMemory(mProgCounter) << 8 | readMemory(mProgCounter + 1); }
	auto  NNN()  const { return mInstruction & 0xFFF; }
	auto  NN0()  const { return mInstruction & 0xFF0; }
	auto& VX() { return mRegisterV[(mInstruction >> 8) & 0xF]; }

	std::string hexOpcode(u32) const;

	void initProgramParams(u32, s32);
	void calculateBoostCPF(s32);
	void changeFunctionSet(FncSetInterface*); // to be deprecated

	void setInterrupt(Interrupt);
	void triggerError(std::string_view);
	void triggerOpcodeError(u32);

	void handleFrameWait();
	void handleInputWait();

public:
	explicit VM_Guest(
		HomeDirManager&,
		BasicVideoSpec&,
		BasicAudioSpec&
	);
	~VM_Guest();


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

	bool romCopyToMemory(usz, usz);
	void fontCopyToMemory();

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
