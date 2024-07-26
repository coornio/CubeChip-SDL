/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <optional>
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

class alignas(64) VM_Guest final {

/*==================================================================*/
	#pragma region VM_GUEST SPECIFICS
/*==================================================================*/

private:
	FunctionsForGigachip SetGigachip{ *this }; friend class FunctionsForGigachip;
	u32 __PADDING1[2]{};
	FunctionsForMegachip SetMegachip{ *this }; friend class FunctionsForMegachip;
	u32 __PADDING2[2]{};
	FunctionsForModernXO SetModernXO{ *this }; friend class FunctionsForModernXO;
	FunctionsForLegacySC SetLegacySC{ *this }; friend class FunctionsForLegacySC;
	FunctionsForClassic8 SetClassic8{ *this }; friend class FunctionsForClassic8;
	FncSetInterface* currFncSet{ &SetClassic8 }; // whole segment to be deprecated
	u32 __PADDING3[2]{};

	HomeDirManager& HDM;
	BasicVideoSpec& BVS;
	BasicAudioSpec& BAS;

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

	struct PlatformQuirks final {
		bool clearVF{};
		bool jmpRegX{};
		bool shiftVX{};
		bool idxRegNoInc{};
		bool idxRegMinus{};
		bool waitVblank{};
		bool waitScroll{};
		bool wrapSprite{};
	} Quirk;

	u64  mTotalCycles{};
	u32  mTotalFrames{};

	s32  mCyclesPerFrame{};
	s32  boost{};
	f32  mFramerate{};

	using enum Interrupt;
	Interrupt mInterruptType{ CLEAR };

	bool mSystemPaused{};

public:
	[[nodiscard]]
	bool isSystemPaused(void) const;
	void isSystemPaused(bool);

	bool islegacyPlatform() const {
		return State.chip8E_rom
			|| State.chip8X_rom
			|| State.schip_legacy
			|| State.chip8_legacy;
	}

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

public:
	explicit VM_Guest(
		HomeDirManager&,
		BasicVideoSpec&,
		BasicAudioSpec&
	);
	~VM_Guest();

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

	u8   mRegisterV[16]{};

	std::vector<u8>
		mMemoryBank{};

	//u32* mStackTop{ mStackBank };
	u32  mStackTop{};
	u32  mRegisterI{};

	alignas(64) u32
		mStackBank[16]{};

	Map2D<u8>  displayBuffer[4];
	Map2D<u32> color8xBuffer;

	Map2D<u32> foregroundBuffer;
	Map2D<u32> backgroundBuffer;
	Map2D<u8>  collisionPalette;

	std::vector<u32> megaPalette;

	bool in_range(const usz pos) const noexcept { return pos < mMemoryBank.size(); }
	auto peekStackHead() const { return mStackTop; }

	auto  NNNN() const { return readMemory(mProgCounter) << 8 | readMemory(mProgCounter + 1); }
	auto  NNN()  const { return mInstruction & 0xFFF; }
	auto  NN0()  const { return mInstruction & 0xFF0; }
	auto& VX() { return mRegisterV[(mInstruction >> 8) & 0xF]; }

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

	u32 __PADDING4[1]{};
	f32  mWavePhase{};

	struct Audio_C8 final {
		f32  mTone{};
	} C8;
	struct Audio_XO final {
		explicit Audio_XO(const BasicAudioSpec&);
		const f32 mStep;
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
	#pragma region DISPLAY & TEXTURE
/*==================================================================*/

private:
	struct Traits {
		bool _isLoresExtended{};
		bool _isManualRefresh{};
		bool _isPixelTrailing{};
		bool _isPixelBitColor{};

		s32 W{}, H{};
		s32 Wb{}, Hb{};
		s32 S{};

		s32 pageGuard{};
		s32 maskPlane{ 1 };
		s32 mask8X{ 0xFC };

		using enum BrushType;
		BrushType paintBrush{ XOR };
	} Trait;

	[[nodiscard]] bool isLoresExtended() const { return Trait._isLoresExtended; }
	[[nodiscard]] bool isManualRefresh() const { return Trait._isManualRefresh; }
	[[nodiscard]] bool isPixelTrailing() const { return Trait._isPixelTrailing; }
	[[nodiscard]] bool isPixelBitColor() const { return Trait._isPixelBitColor; }
	void isLoresExtended(const bool state) { Trait._isLoresExtended = state; }
	void isManualRefresh(const bool state) { Trait._isManualRefresh = state; }
	void isPixelTrailing(const bool state) { Trait._isPixelTrailing = state; }
	void isPixelBitColor(const bool state) { Trait._isPixelBitColor = state; }

	struct Video_Texture {
		s32 W{}, H{};

		u8   collision{ 0xFF };
		u8   rgbmod{};
		bool rotate{};
		bool flip_X{};
		bool flip_Y{};
		bool invert{};
		bool nodraw{};
		bool uneven{};
		f32  alpha{ 1.0f };
		u32 __PADDING5[11]{};
	} Texture;

	void setTextureFlags(const usz bits) {
		Texture.rotate = bits >> 0 & 0x1; // false: as-is | true: 90° clockwise
		Texture.flip_X = bits >> 1 & 0x1; // flip on the X axis (rotation agnostic)
		Texture.flip_Y = bits >> 2 & 0x1; // flip on the Y axis (rotation agnostic)
		Texture.invert = bits >> 3 & 0x1; // invert RGB channels
		Texture.rgbmod = bits >> 4 & 0x7; // RGB channel swaps | sepia/grayscale
		Texture.nodraw = bits >> 7 * 0x1; // disable drawing, palette index only
		Texture.uneven = Texture.rotate && (Texture.W != Texture.H);
	}

	struct Video_Colors {
		static constexpr u32 BitColors[]{ // 0-1 classic8, 0-15 modernXO
			0x0C1218, 0xE4DCD4, 0x8C8884, 0x403C38,
			0xD82010, 0x40D020, 0x1040D0, 0xE0C818,
			0x501010, 0x105010, 0x50B0C0, 0xF08010,
			0xE06090, 0xE0F090, 0xB050F0, 0x704020,
		};
		static constexpr u32 ForeColors[]{ // 8X foreground
			0x000000, 0xEE1111, 0x1111EE, 0xEE11EE,
			0x11EE11, 0xEEEE11, 0x11EEEE, 0xEEEEEE,
		};
		static constexpr u32 BackColors[]{ // 8X background
			0x111133, 0x111111, 0x113311, 0x331111,
		};

		u32 bit[16]{}; // pixel bit color (planes)
		u32 hex[10]{}; // mega char sprite gradient
		u32 buzz[2]{}; // colors for buzz glow
		u32 fade[3]{}; // pixel fade color (frames)
		u32 bgindex{}; // background color cycle index
	} Color;

	void _initBitColors() {
		for (auto i{ 0 }; i < 16; ++i) {
			Color.bit[i] = Color.BitColors[i];
		}
		Color.buzz[0] = Color.bit[0];
		Color.buzz[1] = Color.bit[1];
	}

	void _initHexColors() {
		static constexpr auto r{ 0xFF }, g{ 0xFF }, b{ 0xFF };

		for (auto i{ 0 }; i < 10; ++i) {
			const float mult{ 1.0f - 0.045f * i };
			const float R{ r * mult * 1.03f };
			const float G{ g * mult * 1.14f };
			const float B{ b * mult * 1.21f };

			Color.hex[i] = 0xFF000000
				| static_cast<u32>(std::min(std::roundf(R), 255.0f)) << 16
				| static_cast<u32>(std::min(std::roundf(G), 255.0f)) << 8
				| static_cast<u32>(std::min(std::roundf(B), 255.0f));
		}
	}

	void setColorBit332(const usz idx, const usz color) {
		static constexpr u8 map3b[]{ 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xFF };
		static constexpr u8 map2b[]{ 0x00,             0x60,       0xA0,       0xFF };

		Color.bit[idx & 0xF] = map3b[color >> 5 & 0x7] << 16 // red
			| map3b[color >> 2 & 0x7] << 8 // green
			| map2b[color & 0x3];           // blue
	}

	void setBackgroundColorTo(u32* const pColors) const {
		if (pColors) { *pColors = Color.bit[0]; }
	}

	void setBackgroundColorTo(u32* const pColors, const u32 color) const {
		if (pColors) { *pColors = color; }
	}

	void cycleBackgroundColor(u32* const pColors) {
		setBackgroundColorTo(pColors, Color.BackColors[Color.bgindex++ & 0x3]);
	}

	auto getForegroundColor8X(const s32 idx) const {
		return Color.ForeColors[idx & 0x7];
	}

/*==================================================================*/
	#pragma endregion
/*==================================================================*/

	Well512  Wrand;
	HexInput Input;

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
