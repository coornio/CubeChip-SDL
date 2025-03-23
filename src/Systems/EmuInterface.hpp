/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <shared_mutex>
#include <execution>
#include <iostream>
#include <utility>
#include <fstream>
#include <numeric>
#include <cstring>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <format>
#include <array>
#include <cmath>
#include <mutex>
#include <span>
#include <bit>

#include "../Assistants/Typedefs.hpp"
#include "../Assistants/Concepts.hpp"
#include "../Assistants/Misc.hpp"
#include "../Assistants/RGBA.hpp"

#include "GameFileChecker.hpp"

/*==================================================================*/

enum EmuState {
	NORMAL = 0x0, // normal operation
	HIDDEN = 0x1, // window is hidden
	PAUSED = 0x2, // paused by hotkey
	HALTED = 0x4, // normal end path
	FATAL  = 0x8, // fatal error path
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

class BasicKeyboard;
class FrameLimiter;
class Well512;

/*==================================================================*/

class EmuInterface {
	static inline Atom<u32>
		mGlobalState{ EmuState::NORMAL };

protected:
	static inline HomeDirManager* HDM{};
	static inline BasicVideoSpec* BVS{};
	static inline Well512*        RNG{};

	std::jthread mCoreThread;
	AtomSharedPtr<Str> mStatistics;

public:
	void startWorker() noexcept;
	void stopWorker() noexcept;

protected:
	void threadEntry(std::stop_token token);

	std::unique_ptr<FrameLimiter>  Pacer;
	std::unique_ptr<BasicKeyboard> Input;

	u64 mElapsedCycles{};
	f32 mTargetFPS{};
	s32 mTargetCPF{};

public:
	EmuInterface() noexcept;
	virtual ~EmuInterface() noexcept;

	static void assignComponents(
		HomeDirManager* const pHDM,
		BasicVideoSpec* const pBVS
	) noexcept {
		HDM = pHDM;
		BVS = pBVS;
	}

	static void addSystemState(EmuState state) noexcept { mGlobalState.fetch_or ( state, mo::acq_rel); }
	static void subSystemState(EmuState state) noexcept { mGlobalState.fetch_and(~state, mo::acq_rel); }
	static void xorSystemState(EmuState state) noexcept { mGlobalState.fetch_xor( state, mo::acq_rel); }

	static void setSystemState(EmuState state) noexcept { mGlobalState.store(state, mo::release); }
	static auto getSystemState()               noexcept { return mGlobalState.load(mo::acquire);  }

	virtual s32 getMaxDisplayW() const noexcept = 0;
	virtual s32 getMaxDisplayH() const noexcept = 0;
	virtual s32 getDisplaySize() const noexcept { return getMaxDisplayW() * getMaxDisplayH(); }

protected:
	void setSystemFramerate(f32 value) noexcept;
	virtual void processFrame() = 0;

	virtual s32 getCPF()         const noexcept = 0;
	virtual s32 addCPF(s32 delta)      noexcept = 0;

public:
	[[nodiscard]]
	virtual bool isSystemStopped() const noexcept = 0;
	virtual bool isCoreStopped()   const noexcept = 0;

protected:
	virtual void writeStatistics();
public:
	Str fetchStatistics() const noexcept;
};
