/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <execution>
#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <format>
#include <array>
#include <span>
#include <bit>

#include "../Assistants/Typedefs.hpp"

#include "GameFileChecker.hpp"

/*==================================================================*/

enum EmuState : u8 {
	NORMAL = 0x0, // normal operation
	HIDDEN = 0x1, // window is hidden
	PAUSED = 0x2, // paused by hotkey
	HALTED = 0x4, // normal/error end
	FAILED = 0x8, // failed core init
};

struct SimpleKeyMapping {
	u32          idx; // index value associated with entry
	SDL_Scancode key; // primary key mapping
	SDL_Scancode alt; // alternative key mapping
};

class HomeDirManager;
class BasicVideoSpec;
class BasicAudioSpec;
class AudioSpecBlock;
class Well512;

/*==================================================================*/

class EmuInterface {
	static u32 mGlobalState;

protected:
	static HomeDirManager* HDM;
	static BasicVideoSpec* BVS;
	static BasicAudioSpec* BAS;

	static Well512* Wrand;

public:
	EmuInterface() noexcept;
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
	virtual s32 getCPF()       const noexcept = 0;
	virtual f32 getFramerate() const noexcept = 0;
	virtual s32 changeCPF(const s32 delta) noexcept = 0;

	[[nodiscard]]
	virtual bool isSystemStopped() const noexcept = 0;
	virtual bool isCoreStopped()   const noexcept = 0;

	virtual void processFrame() = 0;
};
