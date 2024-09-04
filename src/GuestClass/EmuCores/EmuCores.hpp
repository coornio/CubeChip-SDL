/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../../Assistants/Well512.hpp"
#include "../../Assistants/Map2D.hpp"
#include "../../Types.hpp"

#include "../GameFileChecker.hpp"
#include "../HexInput.hpp"

#include <filesystem>
#include <utility>
#include <memory>
#include <string>

enum EmuState : u8 {
	NORMAL = 0x0, // normal operation
	HIDDEN = 0x1, // window is hidden
	PAUSED = 0x2, // paused by hotkey
	HALTED = 0x4, // normal/error end
	FAILED = 0x8, // failed core init
};

class HomeDirManager;
class BasicVideoSpec;
class BasicAudioSpec;

class EmuInterface {
	static u32 mGlobalState;

protected:
	static HomeDirManager* HDM;
	static BasicVideoSpec* BVS;
	static BasicAudioSpec* BAS;

public:
	virtual ~EmuInterface() noexcept;

	static void assignComponents(
		HomeDirManager* const pHDM,
		BasicVideoSpec* const pBVS,
		BasicAudioSpec* const pBAS
	) noexcept {
		HDM = pHDM;
		BVS = pBVS;
		BAS = pBAS;
	}

	static void addSystemState(const EmuState state) noexcept { mGlobalState |=  state; }
	static void subSystemState(const EmuState state) noexcept { mGlobalState &= ~state; }
	static void xorSystemState(const EmuState state) noexcept { mGlobalState ^=  state; }

	static void setSystemState(const EmuState state) noexcept { mGlobalState  =  state; }
	static auto getSystemState()                     noexcept { return mGlobalState;    }

	virtual u32 getTotalFrames() const noexcept = 0;
	virtual u64 getTotalCycles() const noexcept = 0;
	virtual s32 fetchCPF()       const noexcept = 0;
	virtual f32 fetchFramerate() const noexcept = 0;
	virtual s32 changeCPF(const s32 delta) noexcept = 0;

	[[nodiscard]]
	virtual bool isSystemStopped() const noexcept = 0;
	virtual bool isCoreStopped()   const noexcept = 0;

	virtual void processFrame() = 0;
};

/*==================================================================*/

class Chip8_CoreInterface : public EmuInterface {
	static std::filesystem::path* sPermaRegsPath;
	static std::filesystem::path* sSavestatePath;

protected:
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

	enum class Interrupt {
		CLEAR, // no interrupt
		FRAME, // single-frame
		SOUND, // wait for sound and stop
		DELAY, // wait for delay and proceed
		INPUT, // wait for input and proceed
		FINAL, // end state, all is well
		ERROR, // end state, error occured
	};

	Interrupt mInterruptType{};

	f32  mFramerate{};

	u64  mTotalCycles{};
	u32  mTotalFrames{};

	s32  mCyclesPerFrame{};
	s32  boost{};

	s32 mDisplaySize{};
	s32 mDisplayW{},  mDisplayH{};
	s32 mDisplayWb{}, mDisplayHb{};

	void setDisplayResolution(const s32 W, const s32 H) noexcept {
		mDisplaySize = W * H;
		mDisplayW = W; mDisplayWb = W - 1;
		mDisplayH = H; mDisplayHb = H - 1;
	}

	u8 mCoreState{ EmuState::NORMAL };

	void addCoreState(const EmuState state) noexcept { mCoreState |=  state; }
	void subCoreState(const EmuState state) noexcept { mCoreState &= ~state; }
	void xorCoreState(const EmuState state) noexcept { mCoreState ^=  state; }

	void setCoreState(const EmuState state) noexcept { mCoreState = state; }
	auto getCoreState()               const noexcept { return mCoreState;  }

	bool isSystemStopped() const noexcept override { return getCoreState() || getSystemState(); }
	bool isCoreStopped()   const noexcept override { return getCoreState(); }

	bool mLoresExtended{};
	bool mManualRefresh{};
	bool mPixelTrailing{};

	[[nodiscard]] bool isLoresExtended() const noexcept { return mLoresExtended; }
	[[nodiscard]] bool isManualRefresh() const noexcept { return mManualRefresh; }
	[[nodiscard]] bool isPixelTrailing() const noexcept { return mPixelTrailing; }
	void isLoresExtended(const bool state) noexcept { mLoresExtended = state; }
	void isManualRefresh(const bool state) noexcept { mManualRefresh = state; }
	void isPixelTrailing(const bool state) noexcept { mPixelTrailing = state; }

	Well512  Wrand;
	HexInput Input;

	std::string formatOpcode(const u32 OP) const;

	void triggerInterrupt(const Interrupt type) noexcept;
	void triggerCritError(const std::string& msg) noexcept;
	void instructionError(const u32 HI, const u32 LO);

	void copyGameToMemory(u8* dest, const u32 offset) noexcept;
	void copyFontToMemory(u8* dest, const u32 offset, const u32 size) noexcept;

	virtual void handlePreFrameInterrupt() noexcept = 0;
	virtual void handleEndFrameInterrupt() noexcept = 0;

	virtual void handleTimerTick() noexcept = 0;
	virtual void instructionLoop() noexcept = 0;

	virtual void renderAudioData() = 0;
	virtual void renderVideoData() = 0;

public:
	explicit Chip8_CoreInterface() noexcept;

	void processFrame() override;

	u32 getTotalFrames() const noexcept override { return mTotalFrames; }
	u64 getTotalCycles() const noexcept override { return mTotalCycles; }

	s32 fetchCPF()       const noexcept override { return mCyclesPerFrame; }
	f32 fetchFramerate() const noexcept override { return mFramerate; }

	s32 changeCPF(const s32 delta) noexcept override {
		if (stateRunning() && !stateWaiting()) {
			mCyclesPerFrame += mCyclesPerFrame > 0
				? delta : -delta;
		}
		return mCyclesPerFrame;
	}

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

	static constexpr u8 cFontData[]{
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
	};
	static constexpr u8 cFontDataMega[]{
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, // 0
		0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, // 1
		0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF, // 2
		0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C, // 3
		0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06, // 4
		0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C, // 5
		0x3E, 0x7C, 0xC0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C, // 6
		0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60, // 7
		0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C, // 8
		0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C, // 9
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, // 0
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, // 0
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, // 0
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, // 0
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, // 0
		0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C  // 0
	};

	static constexpr u32 cBitsColor[]{ // 0-1 monochrome, 0-15 palette color
		0x0C1218, 0xE4DCD4, 0x8C8884, 0x403C38,
		0xD82010, 0x40D020, 0x1040D0, 0xE0C818,
		0x501010, 0x105010, 0x50B0C0, 0xF08010,
		0xE06090, 0xE0F090, 0xB050F0, 0x704020,
	};
	static constexpr u32 cForeColor[]{ // CHIP-8X foreground colors
		0x000000, 0xEE1111, 0x1111EE, 0xEE11EE,
		0x11EE11, 0xEEEE11, 0x11EEEE, 0xEEEEEE,
	};
	static constexpr u32 cBackColor[]{ // CHIP-8X background colors
		0x111133, 0x111111, 0x113311, 0x331111,
	};
};
