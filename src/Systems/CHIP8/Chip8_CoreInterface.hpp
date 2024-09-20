/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../EmuInterface.hpp"

/*==================================================================*/

class Chip8_CoreInterface : public EmuInterface {

protected:
	static fsPath* sPermaRegsPath;
	static fsPath* sSavestatePath;

	std::unique_ptr<AudioSpecBlock> ASB;

	std::vector<SimpleKeyMapping> mCustomBinds;

private:
	u32  mTickLast{};
	u32  mTickSpan{};

	u32  mKeysCurr{}; // bitfield of key states in current frame
	u32  mKeysPrev{}; // bitfield of key states in previous frame
	u32  mKeysLock{}; // bitfield of keys excluded from input checks
	u32  mKeysLoop{}; // bitfield of keys repeating input on Fx0A

protected:
	void loadPresetBinds();
	void loadCustomBinds(std::span<const SimpleKeyMapping> binds);

	void updateKeyStates();

	bool keyPressed(u8* returnKey, u32 tickCount) noexcept;
	bool keyHeld_P1(u32 keyIndex) const noexcept;
	bool keyHeld_P2(u32 keyIndex) const noexcept;

/*==================================================================*/

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

	struct PlatformTraits final {
		u32  mCoreState{ EmuState::NORMAL };
		bool mDisplayLarger{};
		bool mManualRefresh{};
		bool mPixelTrailing{};
		bool mBuzzerEnabled{};
	} Trait;

	enum class Interrupt {
		CLEAR, // no interrupt
		FRAME, // single-frame
		SOUND, // wait for sound and stop
		DELAY, // wait for delay and proceed
		INPUT, // wait for input and proceed
		FINAL, // end state, all is well
		ERROR, // end state, error occured
	};

	enum class Resolution {
		ERROR,
		HI, // 128 x  64 - 2:1
		LO, //  64 x  32 - 2:1
		TP, //  64 x  64 - 2:1
		FP, //  64 x 128 - 2:1
		MC, // 256 x 192 - 4:3
	};

/*==================================================================*/

	void addCoreState(const EmuState state) noexcept { Trait.mCoreState |=  state; }
	void subCoreState(const EmuState state) noexcept { Trait.mCoreState &= ~state; }
	void xorCoreState(const EmuState state) noexcept { Trait.mCoreState ^=  state; }

	void setCoreState(const EmuState state) noexcept { Trait.mCoreState = state; }
	auto getCoreState()               const noexcept { return Trait.mCoreState;  }

	bool isSystemStopped() const noexcept override { return getCoreState() || getSystemState(); }
	bool isCoreStopped()   const noexcept override { return getCoreState(); }

	bool isDisplayLarger() const noexcept { return Trait.mDisplayLarger; }
	bool isManualRefresh() const noexcept { return Trait.mManualRefresh; }
	bool isPixelTrailing() const noexcept { return Trait.mPixelTrailing; }
	bool isBuzzerEnabled() const noexcept { return Trait.mBuzzerEnabled; }
	void isDisplayLarger(const bool state) noexcept { Trait.mDisplayLarger = state; }
	void isManualRefresh(const bool state) noexcept { Trait.mManualRefresh = state; }
	void isPixelTrailing(const bool state) noexcept { Trait.mPixelTrailing = state; }
	void isBuzzerEnabled(const bool state) noexcept { Trait.mBuzzerEnabled = state; }

/*==================================================================*/

	u64 mTotalCycles{};
	u32 mTotalFrames{};

	s32 mActiveCPF{};
	f32 mFramerate{};

	Interrupt mInterrupt{};

	s32 mDisplayW{},  mDisplayH{};
	s32 mDisplayWb{}, mDisplayHb{};

	void setDisplayResolution(const s32 W, const s32 H) noexcept {
		mDisplayW = W; mDisplayWb = W - 1;
		mDisplayH = H; mDisplayHb = H - 1;
	}

	f32 mBuzzerTone{};
	u32 mPlanarMask{ 0x1 };

	u32 mCurrentPC{};
	u32 mRegisterI{};

	u32 mDelayTimer{};
	u32 mSoundTimer{};

	u32 mStackTop{};
	u8* mInputReg{};

	u8  mRegisterV[16]{};
	u32 mStackBank[16]{};

/*==================================================================*/

	std::string formatOpcode(const u32 OP) const;
	void instructionError(const u32 HI, const u32 LO);

	void triggerInterrupt(const Interrupt type) noexcept;
	void triggerCritError(const std::string& msg) noexcept;

	bool setPermaRegs(const s32 X) noexcept;
	bool getPermaRegs(const s32 X) noexcept;

	void copyGameToMemory(u8* dest, const u32 offset) noexcept;
	void copyFontToMemory(u8* dest, const u32 offset, const u32 size) noexcept;
	void copyColorsToCore(u32* dest, const u32 size) noexcept;

	virtual void handlePreFrameInterrupt() noexcept;
	virtual void handleEndFrameInterrupt() noexcept;

	virtual void handleTimerTick() noexcept;
	virtual void instructionLoop() noexcept = 0;

	virtual void nextInstruction() noexcept;
	virtual void skipInstruction() noexcept;
	virtual void performProgJump(const u32 next) noexcept;
	virtual void prepDisplayArea(const Resolution mode) = 0;

	virtual void renderAudioData() = 0;
	virtual void renderVideoData() = 0;

	f32  calcBuzzerTone() const noexcept;

public:
	Chip8_CoreInterface() noexcept;
	~Chip8_CoreInterface() noexcept;

	void processFrame() override;

	u32 getTotalFrames() const noexcept override { return mTotalFrames; }
	u64 getTotalCycles() const noexcept override { return mTotalCycles; }

	s32 getCPF()       const noexcept override { return mActiveCPF; }
	f32 getFramerate() const noexcept override { return mFramerate; }

	s32 changeCPF(const s32 delta) noexcept override {
		if (stateRunning() && !stateWaiting()) {
			mActiveCPF += mActiveCPF > 0
				? delta : -delta;
		}
		return mActiveCPF;
	}

	bool stateRunning() const noexcept { return (
		mInterrupt != Interrupt::FINAL &&
		mInterrupt != Interrupt::ERROR
	); }
	bool stateStopped() const noexcept { return (
		mInterrupt == Interrupt::FINAL ||
		mInterrupt == Interrupt::ERROR
	); }
	bool stateWaitKey() const noexcept { return (
		mInterrupt == Interrupt::INPUT
	); }
	bool stateWaiting() const noexcept { return (
		mInterrupt == Interrupt::SOUND ||
		mInterrupt == Interrupt::DELAY ||
		mInterrupt == Interrupt::INPUT
	); }

protected:
	static           std::array<u8, 240> sFontsData;
	static constexpr std::array<u8, 240> cFontsData{ {
		0x60, 0xA0, 0xA0, 0xA0, 0xC0, // 0
		0x40, 0xC0, 0x40, 0x40, 0xE0, // 1
		0xC0, 0x20, 0x40, 0x80, 0xE0, // 2
		0xC0, 0x20, 0x40, 0x20, 0xC0, // 3
		0x20, 0xA0, 0xE0, 0x20, 0x20, // 4
		0xE0, 0x80, 0xC0, 0x20, 0xC0, // 5
		0x40, 0x80, 0xC0, 0xA0, 0x40, // 6
		0xE0, 0x20, 0x60, 0x40, 0x40, // 7
		0x40, 0xA0, 0x40, 0xA0, 0x40, // 8
		0x40, 0xA0, 0x60, 0x20, 0x40, // 9
		0x40, 0xA0, 0xE0, 0xA0, 0xA0, // A
		0xC0, 0xA0, 0xC0, 0xA0, 0xC0, // B
		0x60, 0x80, 0x80, 0x80, 0x60, // C
		0xC0, 0xA0, 0xA0, 0xA0, 0xC0, // D
		0xE0, 0x80, 0xC0, 0x80, 0xE0, // E
		0xE0, 0x80, 0xC0, 0x80, 0x80, // F

		0x7C, 0xC6, 0xCE, 0xDE, 0xD6, 0xF6, 0xE6, 0xC6, 0x7C, 0x00, // 0
		0x10, 0x30, 0xF0, 0x30, 0x30, 0x30, 0x30, 0x30, 0xFC, 0x00, // 1
		0x78, 0xCC, 0xCC, 0x0C, 0x18, 0x30, 0x60, 0xCC, 0xFC, 0x00, // 2
		0x78, 0xCC, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0xCC, 0x78, 0x00, // 3
		0x0C, 0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x0C, 0x1E, 0x00, // 4
		0xFC, 0xC0, 0xC0, 0xC0, 0xF8, 0x0C, 0x0C, 0xCC, 0x78, 0x00, // 5
		0x38, 0x60, 0xC0, 0xC0, 0xF8, 0xCC, 0xCC, 0xCC, 0x78, 0x00, // 6
		0xFE, 0xC6, 0xC6, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00, // 7
		0x78, 0xCC, 0xCC, 0xEC, 0x78, 0xDC, 0xCC, 0xCC, 0x78, 0x00, // 8
		0x7C, 0xC6, 0xC6, 0xC6, 0x7C, 0x18, 0x18, 0x30, 0x70, 0x00, // 9
		/* ------ omit segment below if legacy superchip ------ */
		0x30, 0x78, 0xCC, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0xCC, 0x00, // A
		0xFC, 0x66, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x66, 0xFC, 0x00, // B
		0x3C, 0x66, 0xC6, 0xC0, 0xC0, 0xC0, 0xC6, 0x66, 0x3C, 0x00, // C
		0xF8, 0x6C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x6C, 0xF8, 0x00, // D
		0xFE, 0x62, 0x60, 0x64, 0x7C, 0x64, 0x60, 0x62, 0xFE, 0x00, // E
		0xFE, 0x66, 0x62, 0x64, 0x7C, 0x64, 0x60, 0x60, 0xF0, 0x00, // F
	} };
	static           std::array<u32, 16> sBitColors;
	static constexpr std::array<u32, 16> cBitColors{ { // 0-1 monochrome, 0-15 palette color
		0x0C1218, 0xE4DCD4, 0x8C8884, 0x403C38,
		0xD82010, 0x40D020, 0x1040D0, 0xE0C818,
		0x501010, 0x105010, 0x50B0C0, 0xF08010,
		0xE06090, 0xE0F090, 0xB050F0, 0x704020,
	} };

	static constexpr std::array<u32,  8> cForeColor{ { // CHIP-8X foreground colors
		0x000000, 0xEE1111, 0x1111EE, 0xEE11EE,
		0x11EE11, 0xEEEE11, 0x11EEEE, 0xEEEEEE,
	} };
	static constexpr std::array<u32,  4> cBackColor{ { // CHIP-8X background colors
		0x111133, 0x111111, 0x113311, 0x331111,
	} };
};
