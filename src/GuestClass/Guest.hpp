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
	FunctionsForGigachip SetGigachip{ *this }; friend class FunctionsForGigachip;
	FunctionsForMegachip SetMegachip{ *this }; friend class FunctionsForMegachip;
	FunctionsForModernXO SetModernXO{ *this }; friend class FunctionsForModernXO;
	FunctionsForLegacySC SetLegacySC{ *this }; friend class FunctionsForLegacySC;
	FunctionsForClassic8 SetClassic8{ *this }; friend class FunctionsForClassic8;
	FncSetInterface* currFncSet{ &SetClassic8 }; // whole segment to be deprecated

	HomeDirManager& HDM;
	BasicVideoSpec& BVS;
	BasicAudioSpec& BAS;

	HexInput Input;
	Well512  Wrand;

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

	bool islegacyPlatform() const {
		return State.chip8E_rom
			|| State.chip8X_rom
			|| State.schip_legacy
			|| State.chip8_legacy;
	}

public:
	explicit VM_Guest(
		HomeDirManager&,
		BasicVideoSpec&,
		BasicAudioSpec&
	);
	~VM_Guest();

/*==================================================================*/
	#pragma region VM_GUEST SPECIFICS
/*==================================================================*/

private:
	u64  mTotalCycles{};
	u32  mTotalFrames{};

	s32  mCyclesPerFrame{};
	s32  boost{};
	f32  mFramerate{};

	Interrupt mInterruptType{};

	bool mSystemPaused{};

public:
	[[nodiscard]]
	bool isSystemPaused(void) const;
	void isSystemPaused(bool);

	auto getTotalFrames() const { return mTotalFrames; }
	auto getTotalCycles() const { return mTotalCycles; }

	auto fetchCPF()       const { return mCyclesPerFrame; }
	auto fetchFramerate() const { return mFramerate; }

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

/*==================================================================*/
	#pragma endregion
/*==================================================================*/

/*==================================================================*/
	#pragma region AUDIO GENERATION
/*==================================================================*/

private:
	bool mBuzzLight{};
	bool mAudioIsXO{};
	bool mAudioIsMC{};
	f32  mWavePhase{};

	struct Audio_C8 final {
		f32  mTone{};
	} C8;
	struct Audio_XO final {
		explicit Audio_XO(const BasicAudioSpec&);

		f32  mStep{};
		f32  mTone{};
		u8   mData[16]{};
	} XO{ BAS };
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

/*==================================================================*/
	#pragma region CPU & MEMORY
/*==================================================================*/

private:
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

	std::vector<u32> megaPalette;

	Map2D<u32> foregroundBuffer;
	Map2D<u32> backgroundBuffer;
	Map2D<u8>  collisionPalette;

	Map2D<u8>  displayBuffer[4];
	Map2D<u32> color8xBuffer;

	bool in_range(usz pos) const noexcept;
	u32  peekStackHead() const;

	u32  NNNN() const;
	u32  NNN()  const;
	u32  NN0()  const;
	u8&  VX();

	// Write memory at given index using given value
	void writeMemory(usz value, usz pos);
	// Write memory at saved index using given value
	void writeMemoryI(usz value, usz pos);
	// Write memory at saved index using given value
	void writeMemoryI(usz value);

	// Read memory at given index
	u8   readMemory(usz pos) const;
	// Read memory at saved index
	u8   readMemoryI(usz pos) const;
	// Read memory at saved index
	u8   readMemoryI() const;

/*==================================================================*/
	#pragma endregion
/*==================================================================*/

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
	bool setupMachine();

	void processFrame();

private:
	void initPlatform();
	bool romTypeCheck();

	bool romCopyToMemory(usz, usz);
	void fontCopyToMemory();

	void prepDisplayArea(Resolution, bool = false);
	void renderToTexture();

	void instructionLoop();
};
