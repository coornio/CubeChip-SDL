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
	FunctionsForMegachip SetMegachip{ *this }; friend class FunctionsForMegachip;
	FunctionsForModernXO SetModernXO{ *this }; friend class FunctionsForModernXO;
	FunctionsForLegacySC SetLegacySC{ *this }; friend class FunctionsForLegacySC;
	FunctionsForClassic8 SetClassic8{ *this }; friend class FunctionsForClassic8;
	FncSetInterface* currFncSet{ &SetClassic8 }; // whole segment to be deprecated

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

	bool mSystemStopped{};

public:
	[[nodiscard]]
	bool isSystemStopped(void) const;
	void isSystemStopped(bool);

	bool islegacyPlatform() const {
		return State.chip8E_rom
			|| State.chip8X_rom
			|| State.schip_legacy
			|| State.chip8_legacy;
	}

	auto getTotalFrames() const noexcept { return mTotalFrames; }
	auto getTotalCycles() const noexcept { return mTotalCycles; }

	auto fetchCPF()       const noexcept { return mCyclesPerFrame; }
	auto fetchFramerate() const noexcept { return mFramerate; }

	bool stateRunning() const noexcept { return (
		mInterruptType != Interrupt::FINAL &&
		mInterruptType != Interrupt::ERROR
	); }
	bool stateStopped() const noexcept { return (
		mInterruptType == Interrupt::FINAL ||
		mInterruptType == Interrupt::ERROR
	); }
	bool stateWaitKey() const noexcept { return (
		mInterruptType == Interrupt::INPUT
	); }
	bool stateWaiting() const noexcept { return (
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
	Map2D<u32> megaColorPalette;

	bool in_range(const usz pos) const noexcept { return pos < mMemoryBank.size(); }
	auto peekStackHead() const noexcept { return mStackTop; }

	auto  NNNN() const noexcept { return readMemory(mProgCounter) << 8 | readMemory(mProgCounter + 1); }
	auto  NNN()  const noexcept { return mInstruction & 0xFFF; }
	auto  NN0()  const noexcept { return mInstruction & 0xFF0; }
	auto& VX() noexcept { return mRegisterV[(mInstruction >> 8) & 0xF]; }

	// Write memory at given index using given value
	void writeMemory(const usz value, const usz pos) noexcept {
		//mMemoryBank[pos & mMemoryBank.size() - 1] = static_cast<u8>(value);
		if (in_range(pos)) { mMemoryBank[pos] = static_cast<u8>(value); }
	}
	// Write memory at saved index using given value
	void writeMemoryI(const usz value, const usz pos) noexcept {
		//mMemoryBank[mRegisterI + pos & mMemoryBank.size() - 1] = static_cast<u8>(value);
		if (in_range(mRegisterI + pos)) { mMemoryBank[mRegisterI + pos] = static_cast<u8>(value); }
	}
	// Write memory at saved index using given value
	void writeMemoryI(const usz value) noexcept {
		//mMemoryBank[mRegisterI & mMemoryBank.size() - 1] = static_cast<u8>(value);
		if (in_range(mRegisterI)) { mMemoryBank[mRegisterI] = static_cast<u8>(value); }
	}

	// Read memory at given index
	auto readMemory(const usz pos) const noexcept -> u8 {
		return (in_range(pos)) ? mMemoryBank[pos] : 0;
		//return mMemoryBank[pos & mMemoryBank.size() - 1];
	}
	// Read memory at saved index
	auto readMemoryI(const usz pos) const noexcept -> u8 {
		return (in_range(mRegisterI + pos)) ? mMemoryBank[mRegisterI + pos] : 0;
		//return mMemoryBank[mRegisterI + pos & mMemoryBank.size() - 1];
	}
	// Read memory at saved index
	auto readMemoryI() const noexcept -> u8 {
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

	f32  mWavePhase{};

	struct Audio_C8 final {
		f32  mTone{};
	} C8;
	struct Audio_XO final {
		explicit Audio_XO(const BasicAudioSpec&) noexcept;
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

	void setAudioTone_C8() noexcept;
	void setAudioTone_8X(u8) noexcept;

	void setAudioTone_XO(u8) noexcept;
	void fetchPattern_XO() noexcept;

	void resetAudioTrack(void) noexcept;
	void startAudioTrack(bool) noexcept;

	void renderAudio_C8(std::span<s16>, s16) noexcept;
	void renderAudio_XO(std::span<s16>, s16) noexcept;
	void renderAudio_MC(std::span<s16>, s16) noexcept;

	void renderAudioData();

/*==================================================================*/
	#pragma endregion
/*==================================================================*/

/*==================================================================*/
	#pragma region DISPLAY & TEXTURE
/*==================================================================*/

private:
	struct Video_Traits {
		bool _isLoresExtended{};
		bool _isManualRefresh{};
		bool _isPixelTrailing{};
		bool _isPixelBitColor{};

		s32 W{}, H{};
		s32 Wb{}, Hb{};
		s32 S{};

		u8  opacityMC{ 0xFF };
		u8  pageGuard{ 0x00 };
		u8  maskPlane{ 0x01 };
		u8  mask8X{ 0xFC };

		using enum BrushType;
		BrushType paintBrush{ XOR };
	} Trait;

	[[nodiscard]] bool isLoresExtended() const noexcept { return Trait._isLoresExtended; }
	[[nodiscard]] bool isManualRefresh() const noexcept { return Trait._isManualRefresh; }
	[[nodiscard]] bool isPixelTrailing() const noexcept { return Trait._isPixelTrailing; }
	[[nodiscard]] bool isPixelBitColor() const noexcept { return Trait._isPixelBitColor; }
	void isLoresExtended(const bool state) noexcept { Trait._isLoresExtended = state; }
	void isManualRefresh(const bool state) noexcept { Trait._isManualRefresh = state; }
	void isPixelTrailing(const bool state) noexcept { Trait._isPixelTrailing = state; }
	void isPixelBitColor(const bool state) noexcept { Trait._isPixelBitColor = state; }

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

	void _initBitColors() noexcept {
		for (auto i{ 0 }; i < 16; ++i) {
			Color.bit[i] = Color.BitColors[i];
		}
		Color.buzz[0] = Color.bit[0];
		Color.buzz[1] = Color.bit[1];
	}

	void _initHexColors() noexcept {
		static constexpr auto r{ 0xFF }, g{ 0xFF }, b{ 0xFF };

		for (auto i{ 0 }; i < 10; ++i) {
			const f32 mult{ 1.0f - 0.045f * i };
			const f32 R{ r * mult * 1.03f };
			const f32 G{ g * mult * 1.14f };
			const f32 B{ b * mult * 1.21f };

			Color.hex[i] = 0xFF000000
				| static_cast<u32>(std::min(std::round(R), 255.0f)) << 16
				| static_cast<u32>(std::min(std::round(G), 255.0f)) << 8
				| static_cast<u32>(std::min(std::round(B), 255.0f));
		}
	}

	void setColorBit332(const s32 idx, const s32 color) noexcept {
		static constexpr u8 map3b[]{ 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xFF };
		static constexpr u8 map2b[]{ 0x00,             0x60,       0xA0,       0xFF };

		Color.bit[idx & 0xF] = map3b[color >> 5 & 0x7] << 16 // red
							 | map3b[color >> 2 & 0x7] <<  8 // green
							 | map2b[color      & 0x3];      // blue
	}

	void setBackgroundColorTo(const u32 color) const noexcept;

	void cycleBackgroundColor() noexcept;

	auto getForegroundColor8X(const s32 idx) const noexcept {
		return Color.ForeColors[idx & 0x7];
	}

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

/*==================================================================*/
	#pragma endregion
/*==================================================================*/

	Well512  Wrand;
	HexInput Input;

private:
	bool routineCall(u32) noexcept;
	bool routineReturn() noexcept;

	bool readPermRegs(usz);
	bool writePermRegs(usz);

	void skipInstruction() noexcept; // to be deprecated
	void skipInstruction_C8() noexcept;
	void skipInstruction_MC() noexcept;
	void skipInstruction_XO() noexcept;
	void skipInstruction_HW() noexcept;
	bool jumpInstruction_C8(u32) noexcept;
	bool jumpInstruction_8E(s32) noexcept;

	void flushBuffers(FlushType);
	void setDisplayOpacity(const s32) const;

	std::string hexOpcode(u32) const;

	void initProgramParams(u32, s32) noexcept;
	void calculateBoostCPF(s32) noexcept;
	void changeFunctionSet(FncSetInterface*) noexcept; // to be deprecated

	void setInterrupt(Interrupt);
	void triggerError(std::string_view);
	void triggerOpcodeError(u32);

	void handleFrameWait() noexcept;
	void handleInputWait() noexcept;

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

/*==================================================================*/
	#pragma region 0 instruction branch
/*==================================================================*/

	// 00DN - scroll selected color plane N lines down *XOCHIP*
	void instruction_00CN_XO(const s32 N) {
		if (Quirk.waitScroll) [[unlikely]]
			{ setInterrupt(Interrupt::FRAME); }
		if (N) { currFncSet->scrollDN(N); }
	}
	// 00DN - scroll selected color plane N lines up *XOCHIP*
	void instruction_00DN_XO(const s32 N) {
		if (Quirk.waitScroll) [[unlikely]]
			{ setInterrupt(Interrupt::FRAME); }
		if (N) { currFncSet->scrollUP(N); }
	}
	// 00E0 - push (and then clear) framebuffer to screen *MEGACHIP*
	void instruction_00E0_MC() {
		setInterrupt(Interrupt::FRAME);
		flushBuffers(FlushType::DISPLAY);
	}
	// 00E0 - erase whole display
	void instruction_00E0_C8() {
		if (Quirk.waitVblank) [[unlikely]]
			{ setInterrupt(Interrupt::FRAME); }
		displayBuffer[0].wipeAll();
	}
	// 00E0 - erase all color planes *XO-CHIP*
	void instruction_00E0_XO() {
		for (auto P{ 0 }; P < 4; ++P) {
			if (!(Trait.maskPlane & (1 << P))) { continue; }
			displayBuffer[P].wipeAll();
		}
	}
	// 00E1 - invert selected color planes *HWCHIP64*
	void instruction_00E1_HW() {
		for (auto P{ 0 }; P < 4; ++P) {
			if (!(Trait.maskPlane & (1 << P))) { continue; }
			for (auto& px : displayBuffer[P].span()) { px ^= 1; }
		}
	}
	// 00ED - stop signal *CHIP-8E*
	void instruction_00ED_8E() {
		setInterrupt(Interrupt::SOUND);
	}
	// 00EE - return from subroutine
	void instruction_00EE_C8() {
		if (routineReturn()) [[unlikely]]
			{ triggerError("Error :: Cannot return from empty stack!"); }
	}
	// 00F0 - return from subroutine *CHIP-8X MPD*
	void instruction_00F0_8X_MP() {
		if (routineReturn()) [[unlikely]]
			{ triggerError("Error :: Cannot return from empty stack!"); }
	}
	// 00F1 - set DRAW mode to ADD *HWCHIP64*
	void instruction_00F1_HW() {
		Trait.paintBrush = BrushType::ADD;
	}
	// 00F2 - set DRAW mode to SUB *HWCHIP64*
	void instruction_00F2_HW() {
		Trait.paintBrush = BrushType::SUB;
	}
	// 00F3 - set DRAW mode to XOR *HWCHIP64*
	void instruction_00F3_HW() {
		Trait.paintBrush = BrushType::XOR;
	}
	// 00FB - scroll selected color plane 4 pixels right *XOCHIP*
	void instruction_00FB_XO(const s32 N) {
		if (Quirk.waitScroll) [[unlikely]]
			{ setInterrupt(Interrupt::FRAME); }
		currFncSet->scrollRT(N);
	}
	// 00FC - scroll selected color plane 4 pixels left *XOCHIP*
	void instruction_00FC_XO(const s32 N) {
		if (Quirk.waitScroll) [[unlikely]]
			{ setInterrupt(Interrupt::FRAME); }
		currFncSet->scrollLT(N);
	}
	// 00FD - stop signal *SCHIP*
	void instruction_00FD_C8() {
		setInterrupt(Interrupt::SOUND);
	}
	// 00FE - display == 64*32, erase the screen *XOCHIP*
	void instruction_00FE_C8() {
		prepDisplayArea(Resolution::LO, !State.schip_legacy);
	}
	// 00FF - display == 128*64, erase the screen *XOCHIP*
	void instruction_00FF_C8() {
		prepDisplayArea(Resolution::HI, !State.schip_legacy);
	}



	// 0010 - disable mega mode *MEGACHIP*
	void instruction_0010_MC() {
		setInterrupt(Interrupt::FRAME);
		changeFunctionSet(&SetClassic8);

		isManualRefresh(false);
		resetAudioTrack();

		flushBuffers(FlushType::DISPLAY);
		prepDisplayArea(Resolution::LO);
		setDisplayOpacity(0xFF);
		setBackgroundColorTo(Color.bit[0]);
	}
	// 0011 - enable mega mode *MEGACHIP*
	void instruction_0011_MC() {
		setInterrupt(Interrupt::FRAME);
		changeFunctionSet(&SetMegachip);

		isManualRefresh(true);
		resetAudioTrack();

		flushBuffers(FlushType::DISCARD);
		prepDisplayArea(Resolution::MC);
		setDisplayOpacity(0xFF);
		setBackgroundColorTo(0);
	}
	// 01NN - set I to NN'NNNN *MEGACHIP*
	void instruction_01NN_MC(const s32 NN) {
		mRegisterI = (NN << 16) | NNNN();
		mProgCounter += 2;
	}
	// 02NN - load NN palette colors from RAM at I *MEGACHIP*
	void instruction_02NN_MC(const s32 NN) {
		auto offset{ mRegisterI };
		for (auto pos{ 0 }; pos < NN; offset += 4) {
			megaColorPalette.at_raw(++pos)
				= readMemory(offset + 0) << 24
				| readMemory(offset + 1) << 16
				| readMemory(offset + 2) << 8
				| readMemory(offset + 3);
		}

	}
	// 03NN - set sprite width to NN *MEGACHIP*
	void instruction_03NN_MC(const s32 NN) {
		Texture.W = NN ? NN : 256;
	}
	// 04NN - set sprite height to NN *MEGACHIP*
	void instruction_04NN_MC(const s32 NN) {
		Texture.H = NN ? NN : 256;
	}
	// 05NN - set screen brightness to NN *MEGACHIP*
	void instruction_05NN_MC(const s32 NN) {
		setDisplayOpacity(NN);
	}
	// 060N - start digital sound from RAM at I, repeat if N == 0 *MEGACHIP*
	void instruction_060N_MC(const s32 N) {
		startAudioTrack(N == 0);
	}
	// 0700 - stop digital sound *MEGACHIP*
	void instruction_0700_MC() {
		resetAudioTrack();
	}
	// 080N - set trait flags to VF, blend mode to N *GIGACHIP*
	void instruction_080N_GC(const s32 N) {
		setTextureFlags(mRegisterV[0xF]);
		SetGigachip.chooseBlend(N);
	}
	// 080N - set blend mode to N *MEGACHIP*
	void instruction_080N_MC(const s32 N) {
		static constexpr float alpha[]{ 1.0f, 0.25f, 0.50f, 0.75f };
		Texture.alpha = alpha[N > 3 ? 0 : N];
		SetMegachip.chooseBlend(N);
	}
	// 09NN - set collision color to palette entry NN *MEGACHIP*
	void instruction_09NN_MC(const s32 NN) {
		Texture.collision = static_cast<u8>(NN);
	}


	// 0151 - stop signal if delay timer == 0 *CHIP-8E*
	void instruction_0151_8E() {
		setInterrupt(Interrupt::DELAY);
	}
	// 0188 - skip next instruction *CHIP-8E*
	void instruction_0188_8E() {
		skipInstruction();
	}
	// 0216 - protect pages in V0 *CHIP-8 4PD*
	void instruction_0216_C8_4P() {
		Trait.pageGuard = (3 - (mRegisterV[0] - 1 & 0x3)) << 5;
	}
	// 0200 - erase pages *CHIP-8 4PD*
	void instruction_0200_C8_4P() {
		auto row{ Trait.pageGuard };
		while (row < Trait.H) {
			displayBuffer[0][row++].wipeAll();
		}
	}
	// 0230 - erase pages *CHIP-8 2PD*
	void instruction_0200_C8_2P() {
		auto row{ Trait.pageGuard };
		while (row < Trait.H) {
			displayBuffer[0][row++].wipeAll();
		}
	}
	// 02A0 - cycle background color *CHIP-8X*
	void instruction_02A0_8X() {
		cycleBackgroundColor();
	}
	// 02F0 - cycle background color *CHIP-8X MPD*
	void instruction_02F0_8X_MP() {
		cycleBackgroundColor();
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 1 instruction branch
/*==================================================================*/

	// 1NNN - jump to NNN
	void instruction_1NNN_C8() {
		if (jumpInstruction_C8(NNN())) [[unlikely]]
			{ setInterrupt(Interrupt::SOUND); }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 2 instruction branch
/*==================================================================*/

	// 2NNN - call subroutine at NNN
	void instruction_2NNN_C8() {
		if (routineCall(NNN())) [[unlikely]]
			{ triggerError("Error :: Cannot call with a full stack!"); }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 3 instruction branch
/*==================================================================*/

	// 3XNN - skip next instruction if VX == NN
	void instruction_3xNN_C8(const s32 X, const s32 NN) {
		if (mRegisterV[X] == NN) { skipInstruction(); }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 4 instruction branch
/*==================================================================*/

	// 4XNN - skip next instruction if VX != NN
	void instruction_4xNN_C8(const s32 X, const s32 NN) {
		if (mRegisterV[X] != NN) { skipInstruction(); }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 5 instruction branch
/*==================================================================*/

	// 5XY0 - skip next instruction if VX == VY
	void instruction_5xy0_C8(const s32 X, const s32 Y) {
		if (mRegisterV[X] == mRegisterV[Y]) { skipInstruction(); }
	}
	// 5XY1 - skip next instruction if VX > VY *CHIP-8E*
	void instruction_5xy1_8E(const s32 X, const s32 Y) {
		if (mRegisterV[X] > mRegisterV[Y]) { skipInstruction(); }
	}
	// 5XY1 - add nibbles of VX,VY and modulo 8 to VX *CHIP-8X*
	void instruction_5xy1_8X(const s32 X, const s32 Y) {
		const auto mask{ isLoresExtended() ? 0x77 : 0xFF };
		const auto lenX{ (mRegisterV[X] & 0xF0) + (mRegisterV[Y] & 0xF0) };
		const auto lenY{ (mRegisterV[X] + mRegisterV[Y]) & 0xF };
		mRegisterV[X] = static_cast<u8>((lenX | lenY) & mask);
	}
	// 5XY2 - store range of registers to memory *CHIP-8E*
	void instruction_5xy2_8E(const s32 X, const s32 Y) {
		for (auto Z{ X }; Z <= Y; ++Z) {
			writeMemory(mRegisterV[Z], mRegisterI++);
		}
	}
	// 5XY2 - store range of registers to memory *XOCHIP*
	void instruction_5xy2_XO(const s32 X, const s32 Y) {
		const auto dist{ std::abs(X - Y) + 1 };
		if (X < Y) {
			for (auto Z{ 0 }; Z < dist; ++Z) {
				writeMemoryI(mRegisterV[X + Z], Z);
			}
		} else {
			for (auto Z{ 0 }; Z < dist; ++Z) {
				writeMemoryI(mRegisterV[X - Z], Z);
			}
		}
	}
	// 5XY3 - load range of registers from memory *CHIP-8E*
	void instruction_5xy3_8E(const s32 X, const s32 Y) {
		for (auto Z{ X }; Z <= Y; ++Z) {
			mRegisterV[Z] = readMemory(mRegisterI++);
		}
	}
	// 5XY3 - load range of registers from memory *XOCHIP*
	void instruction_5xy3_XO(const s32 X, const s32 Y) {
		const auto dist{ std::abs(X - Y) + 1 };
		if (X < Y) {
			for (auto Z{ 0 }; Z < dist; ++Z) {
				mRegisterV[X + Z] = readMemoryI(Z);
			}
		} else {
			for (auto Z{ 0 }; Z < dist; ++Z) {
				mRegisterV[X - Z] = readMemoryI(Z);
			}
		}
	}
	// 5XY4 - load range of colors from memory *EXPERIMENTAL*
	void instruction_5xy4_XO(const s32 X, const s32 Y) {
		const auto dist{ std::abs(X - Y) + 1 };
		if (X < Y) {
			for (auto Z{ 0 }; Z < dist; ++Z) {
				setColorBit332(X + Z, readMemoryI(Z));
			}
		} else {
			for (auto Z{ 0 }; Z < dist; ++Z) {
				setColorBit332(X - Z, readMemoryI(Z));
			}
		}
		setBackgroundColorTo(Color.bit[0]);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 6 instruction branch
/*==================================================================*/

	// 6XNN - set VX = NN
	void instruction_6xNN_C8(const s32 X, const s32 NN) {
		mRegisterV[X] = static_cast<u8>(NN);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 7 instruction branch
/*==================================================================*/

	// 7XNN - set VX = VX + NN
	void instruction_7xNN_C8(const s32 X, const s32 NN) {
		mRegisterV[X] += static_cast<u8>(NN);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 8 instruction branch
/*==================================================================*/

	// 8XY0 - set VX = VY
	void instruction_8xy0_C8(const s32 X, const s32 Y) {
		mRegisterV[X] = mRegisterV[Y];
	}
	// 8XY1 - set VX = VX | VY
	void instruction_8xy1_C8(const s32 X, const s32 Y) {
		mRegisterV[X] |= mRegisterV[Y];
		if (Quirk.clearVF) { mRegisterV[0xF] = 0; }
	}
	// 8XY2 - set VX = VX & VY
	void instruction_8xy2_C8(const s32 X, const s32 Y) {
		mRegisterV[X] &= mRegisterV[Y];
		if (Quirk.clearVF) { mRegisterV[0xF] = 0; }
	}
	// 8XY3 - set VX = VX ^ VY
	void instruction_8xy3_C8(const s32 X, const s32 Y) {
		mRegisterV[X] ^= mRegisterV[Y];
		if (Quirk.clearVF) { mRegisterV[0xF] = 0; }
	}
	// 8XY4 - set VX = VX + VY, VF = carry
	void instruction_8xy4_C8(const s32 X, const s32 Y) {
		const auto sum{ mRegisterV[X] + mRegisterV[Y] };
		mRegisterV[X] = static_cast<u8>(sum);
		mRegisterV[0xF] = static_cast<u8>(sum >> 8);
	}
	// 8XY5 - set VX = VX - VY, VF = !borrow
	void instruction_8xy5_C8(const s32 X, const s32 Y) {
		const bool borrow{ mRegisterV[X] < mRegisterV[Y] };
		mRegisterV[X] = mRegisterV[X] - mRegisterV[Y];
		mRegisterV[0xF] = !borrow;
	}
	// 8XY7 - set VX = VY - VX, VF = !borrow
	void instruction_8xy7_C8(const s32 X, const s32 Y) {
		const bool borrow{ mRegisterV[Y] < mRegisterV[X] };
		mRegisterV[X] = mRegisterV[Y] - mRegisterV[X];
		mRegisterV[0xF] = !borrow;
	}
	// 8XY6 - set VX = VY >> 1, VF = carry
	void instruction_8xy6_C8(const s32 X, const s32 Y) {
		if (!Quirk.shiftVX) mRegisterV[X] = mRegisterV[Y];
		const bool lsb{ (mRegisterV[X] & 1) == 1 };
		mRegisterV[X] = mRegisterV[X] >> 1;
		mRegisterV[0xF] = lsb;
	}
	// 8XYE - set VX = VY << 1, VF = carry
	void instruction_8xyE_C8(const s32 X, const s32 Y) {
		if (!Quirk.shiftVX) mRegisterV[X] = mRegisterV[Y];
		const bool msb{ (mRegisterV[X] >> 7) == 1 };
		mRegisterV[X] = mRegisterV[X] << 1;
		mRegisterV[0xF] = msb;
	}
	// 8XYC - set VX = VX * VY, VF = overflow *HWCHIP64*
	void instruction_8xyC_HW(const s32 X, const s32 Y) {
		const auto mul{ mRegisterV[X] * mRegisterV[Y] };
		mRegisterV[X] = static_cast<u8>(mul);
		mRegisterV[0xF] = static_cast<u8>(mul >> 8);
	}
	// 8XYD - set VX = VX / VY, VF = VX % VY *HWCHIP64*
	void instruction_8xyD_HW(const s32 X, const s32 Y) {
		if (!mRegisterV[Y]) {
			mRegisterV[0xF] = mRegisterV[X] = 0;
		} else {
			const auto remainder{ mRegisterV[X] % mRegisterV[Y] };
			mRegisterV[X] = mRegisterV[X] / mRegisterV[Y];
			mRegisterV[0xF] = static_cast<u8>(remainder);
		}
	}
	// 8XYF - set VX = VY / VX, VF = VX % VY *HWCHIP64*
	void instruction_8xyF_HW(const s32 X, const s32 Y) {
		if (!mRegisterV[X]) {
			mRegisterV[0xF] = 0;
		} else {
			const auto remainder{ mRegisterV[Y] % mRegisterV[X] };
			mRegisterV[X] = mRegisterV[Y] / mRegisterV[X];
			mRegisterV[0xF] = static_cast<u8>(remainder);
		}
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 9 instruction branch
/*==================================================================*/

	// 9XY0 - skip next instruction if VX != VY
	void instruction_9xy0_C8(const s32 X, const s32 Y) {
		if (mRegisterV[X] != mRegisterV[Y]) { skipInstruction(); }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region A instruction branch
/*==================================================================*/

	// ANNN - set I = NNN
	void instruction_ANNN_C8() {
		mRegisterI = NNN();
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region B instruction branch
/*==================================================================*/

	// BBNN - jump to current PC - NN *CHIP-8E*
	void instruction_BBNN_8E(const s32 NN) {
		if (jumpInstruction_8E(-NN)) [[unlikely]]
			{ setInterrupt(Interrupt::SOUND); }
	}

	// BFNN - jump to current PC + NN *CHIP-8E*
	void instruction_BFNN_8E(const s32 NN) {
		if (jumpInstruction_8E(+NN)) [[unlikely]]
		{ setInterrupt(Interrupt::SOUND); }
	}

	// BXYN - set foreground color *CHIP-8X*
	void instruction_BxyN_8X(const s32 X, const s32 Y, const s32 N) {
		if (N) {
			currFncSet->drawHiresColor(
				mRegisterV[X], mRegisterV[X + 1], mRegisterV[Y] & 0x7, N
			);
		} else {
			currFncSet->drawLoresColor(
				mRegisterV[X], mRegisterV[X + 1], mRegisterV[Y] & 0x7
			);
		}
		
	}

	// BXNN - jump to NNN + V0 (else VX *SCHIP*)
	void instruction_BxNN_C8(const s32 X) {
		const auto addr{ NNN() + (Quirk.jmpRegX ? mRegisterV[X] : mRegisterV[0]) };
		if (jumpInstruction_C8(addr)) [[unlikely]]
			{ setInterrupt(Interrupt::SOUND); }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region C instruction branch
/*==================================================================*/

	// CXNN - set VX = rnd(256) & NN
	void instruction_CxNN_C8(const s32 X, const s32 NN) {
		mRegisterV[X] = static_cast<u8>(Wrand.get() & NN);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region D instruction branch
/*==================================================================*/

	// DXYN - draw N sprite rows at VX and VY
	void instruction_DxyN_C8(const s32 X, const s32 Y, const s32 N) {
		if (Quirk.waitVblank) [[unlikely]]
			{ setInterrupt(Interrupt::FRAME); }
		currFncSet->drawSprite(X, Y, N);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region E instruction branch
/*==================================================================*/

	// EX9E - skip next instruction if key VX down (p1)
	void instruction_Ex9E_C8(const s32 X) {
		if ( Input.keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
	}
	// EXA1 - skip next instruction if key VX up (p1)
	void instruction_ExA1_C8(const s32 X) {
		if (!Input.keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
	}
	// EXF2 - skip next instruction if key VX down (p2) *CHIP-8X*
	void instruction_ExF2_8X(const s32 X) {
		if ( Input.keyHeld_P2(mRegisterV[X])) { skipInstruction(); }
	}
	// EXF5 - skip next instruction if key VX up (p2) *CHIP-8X*
	void instruction_ExF5_8X(const s32 X) {
		if (!Input.keyHeld_P2(mRegisterV[X])) { skipInstruction(); }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region F instruction branch
/*==================================================================*/

	// F000 - set I = NEXT NNNN then skip instruction *XOCHIP*
	void instruction_F000_XO() {
		mRegisterI = NNNN();
		mProgCounter += 2;
	}
	// F002 - load audio pattern 0..15 from RAM at I..I+15 *XOCHIP*
	void instruction_F002_XO() {
		fetchPattern_XO();
	}
	// F100 - long jump to NEXT NNNN *HWCHIP64*
	void instruction_F100_HW() {
		mProgCounter = NNNN();
	}
	// F200 - call long subroutine *HWCHIP64*
	void instruction_F200_HW() {
		if (routineCall(NNNN())) [[unlikely]]
			{ triggerError("Error :: Cannot call with a full stack!"); }
	}
	// F300 - long jump to NEXT NNNN + V0 *HWCHIP64*
	void instruction_F300_HW() {
		if (jumpInstruction_C8(NNNN() + mRegisterV[0])) [[unlikely]]
			{ setInterrupt(Interrupt::SOUND); }
	}
	// FX01 - set plane drawing to X *XOCHIP*
	void instruction_Fx01_XO(const s32 X) {
		Trait.maskPlane = static_cast<u8>(X);
	}
	// FX03 - load 24bit color X from RAM at I, I+1, I+2 *HWCHIP64*
	void instruction_Fx03_HW(const s32 X) {
		Color.bit[X] = 0xFF000000
			| readMemoryI(0) << 16
			| readMemoryI(1) <<  8
			| readMemoryI(2);
		if (!X) { setBackgroundColorTo(Color.bit[0]); }
	}
	// FX03 - output VX to port 3 *CHIP-8E*
	void instruction_Fx03_8X(const s32  ) {
		setInterrupt(Interrupt::FRAME);
	}
	// FX07 - set VX = delay timer
	void instruction_Fx07_C8(const s32 X) {
		mRegisterV[X] = mDelayTimer;
	}
	// FX0A - set VX = key, wait for keypress
	void instruction_Fx0A_C8(const s32  ) {
		setInterrupt(Interrupt::INPUT);
		if (isManualRefresh()) [[unlikely]] {
			flushBuffers(FlushType::DISPLAY);
		}
	}
	// FX15 - set delay timer = VX
	void instruction_Fx15_C8(const s32 X) {
		mDelayTimer = mRegisterV[X];
	}
	// FX18 - set sound timer = VX
	void instruction_Fx18_C8(const s32 X) {
		if (!State.chip8X_rom) [[likely]]
			{ setAudioTone_C8(); }
		mBuzzLight  = false;
		mSoundTimer = mRegisterV[X] + (mRegisterV[X] == 1);
	}
	// FX1B - skip VX amount of bytes *CHIP-8E*
	void instruction_Fx1B_8E(const s32 X) {
		mProgCounter += mRegisterV[X];
	}
	// FX1E - set I = I + VX
	void instruction_Fx1E_C8(const s32 X) {
		mRegisterI += mRegisterV[X];
	}
	// FX1F - set I = I - VX *HWCHIP64*
	void instruction_Fx1F_HW(const s32 X) {
		mRegisterI -= mRegisterV[X];
	}
	// FX29 - point I to 5 byte hex sprite from value in VX
	void instruction_Fx29_C8(const s32 X) {
		mRegisterI = (mRegisterV[X] & 0xF) * 5;
	}
	// FX30 - point I to 10 byte hex sprite from value in VX *SCHIP*
	void instruction_Fx30_C8(const s32 X) {
		mRegisterI = (mRegisterV[X] & 0xF) * 10 + 80;
	}
	// FX33 - store BCD of VX to RAM at I, I+1, I+2
	void instruction_Fx33_C8(const s32 X) {
		writeMemoryI(mRegisterV[X] / 100, 0);
		writeMemoryI(mRegisterV[X] / 10 % 10, 1);
		writeMemoryI(mRegisterV[X] % 10, 2);
	}
	// FX3A - set sound pitch = VX *XOCHIP*
	void instruction_Fx3A_XO(const s32 X) {
		setAudioTone_XO(mRegisterV[X]);
	}
	// FX4F - set delay timer = VX and wait *CHIP-8E*
	void instruction_Fx4F_8E(const s32 X) {
		setInterrupt(Interrupt::DELAY);
		mDelayTimer = mRegisterV[X];
	}
	// FX55 - store V0..VX to RAM at I..I+X
	void instruction_Fx55_C8(const s32 X) {
		for (auto idx{ 0 }; idx <= X; ++idx)
			{ writeMemoryI(mRegisterV[idx], idx); }
		if (!Quirk.idxRegNoInc) [[likely]]
			{ mRegisterI += X + !Quirk.idxRegMinus; }
	}
	// FX65 - load V0..VX from RAM at I..I+X
	void instruction_Fx65_C8(const s32 X) {
		for (auto idx{ 0 }; idx <= X; ++idx)
			{ mRegisterV[idx] = readMemoryI(idx); }
		if (!Quirk.idxRegNoInc) [[likely]]
			{ mRegisterI += X + !Quirk.idxRegMinus; }
	}
	// FX75 - store V0..VX to the P flags *XOCHIP*
	void instruction_Fx75_C8(const s32 X) {
		if (writePermRegs((State.schip_legacy ? std::min(X, 7) : X) + 1)) [[unlikely]]
			{ triggerError("Error :: Failed writing persistent registers!"); }
	}
	// FX85 - load V0..VX from the P flags *XOCHIP*
	void instruction_Fx85_C8(const s32 X) {
		if (readPermRegs((State.schip_legacy ? std::min(X, 7) : X) + 1)) [[unlikely]]
			{ triggerError("Error :: Failed reading persistent registers!"); }
	}
	// FXE3 - wait for port 3 input, load into VX *CHIP-8E*
	void instruction_FxE3_8E(const s32  ) {
		setInterrupt(Interrupt::FRAME);
	}
	// FXE7 - read port 3 input, load to VX *CHIP-8E*
	void instruction_FxE7_8E(const s32  ) {
		setInterrupt(Interrupt::FRAME);
	}
	// FXF8 - output VX to port (sound freq) *CHIP-8X*
	void instruction_FxF8_8X(const s32 X) {
		setAudioTone_8X(mRegisterV[X]);
	}
	// FXFB - wait for port input, load to VX *CHIP-8X*
	void instruction_FxFB_8X(const s32  ) {
		setInterrupt(Interrupt::FRAME);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
};
