/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <shared_mutex>
#include <functional>
#include <execution>
#include <iostream>
#include <utility>
#include <fstream>
#include <numeric>
#include <numbers>
#include <cstring>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <array>
#include <cmath>
#include <mutex>
#include <span>
#include <bit>

#include "../Assistants/Typedefs.hpp"
#include "../Assistants/Concepts.hpp"
#include "../IncludeMacros/Thread.hpp"
#include "../IncludeMacros/AtomSharedPtr.hpp"
#include "../Assistants/AssignCast.hpp"
#include "../Assistants/ArrayOps.hpp"

#include "../Assistants/AudioDevice.hpp"
#include "../Assistants/Voice.hpp"
#include "../Assistants/FrameLimiter.hpp"
#include "../Assistants/BasicInput.hpp"
#include "../Assistants/Well512.hpp"

#include "CoreRegistry.hpp"

#include <SDL3/SDL_scancode.h>

/*==================================================================*/

enum EmuState {
	NORMAL = 0x00, // normal operation
	HIDDEN = 0x01, // window is hidden
	PAUSED = 0x02, // paused by hotkey
	HALTED = 0x04, // normal end path
	FATAL  = 0x08, // fatal error path
	BENCH  = 0x10, // benchmarking mode

	NOT_RUNNING = HIDDEN | PAUSED | HALTED | FATAL // mask
};

struct SimpleKeyMapping {
	u32          idx; // index value associated with entry
	SDL_Scancode key; // primary key mapping
	SDL_Scancode alt; // alternative key mapping
};

class HomeDirManager;
class BasicVideoSpec;
class GlobalAudioBase;

/*==================================================================*/

class SystemInterface {

	Thread mCoreThread;
	
	Str mOverlayDataBuffer{};
	AtomSharedPtr<Str>
		mOverlayData;

protected:
	Str* getOverlayDataBuffer() noexcept
		{ return &mOverlayDataBuffer; }

protected:
	static inline HomeDirManager* HDM{};
	static inline BasicVideoSpec* BVS{};

	Well512*       RNG{};
	FrameLimiter*  Pacer{};
	BasicKeyboard* Input{};

public:
	void startWorker() noexcept;
	void stopWorker() noexcept;

protected:
	void threadEntry(StopToken token);

	s32 mTargetCPF{};
	Atom<f32> mTargetFPS{};
	Atom<u32> mGlobalState{ EmuState::NORMAL };

protected:
	SystemInterface() noexcept;

public:
	virtual ~SystemInterface() noexcept = default;

public:
	static void assignComponents(
		HomeDirManager* const pHDM,
		BasicVideoSpec* const pBVS
	) noexcept {
		HDM = pHDM;
		BVS = pBVS;
	}

	void addSystemState(EmuState state) noexcept { mGlobalState.fetch_or ( state, mo::acq_rel); }
	void subSystemState(EmuState state) noexcept { mGlobalState.fetch_and(~state, mo::acq_rel); }
	void xorSystemState(EmuState state) noexcept { mGlobalState.fetch_xor( state, mo::acq_rel); }

	void setSystemState(EmuState state) noexcept { mGlobalState.store(state, mo::release); }
	auto getSystemState()         const noexcept { return mGlobalState.load(mo::acquire);  }
	bool isSystemRunning()        const noexcept { return !(getSystemState() & EmuState::NOT_RUNNING); }

	virtual s32 getMaxDisplayW() const noexcept = 0;
	virtual s32 getMaxDisplayH() const noexcept = 0;
	virtual s32 getDisplaySize() const noexcept { return getMaxDisplayW() * getMaxDisplayH(); }

	f32  getSystemFramerate() const noexcept;
	void setSystemFramerate(f32 value) noexcept;

protected:
	void setViewportSizes(bool cond, u32 W, u32 H, u32 mult, u32 ppad) noexcept;

	void setDisplayBorderColor(u32 color) noexcept;

	virtual void mainSystemLoop() = 0;
	
	/**
	 * @brief Save the Overlay data buffer contents to the public-facing buffer as a final step.
	 * @param[in] data :: A pointer to a string object, typically the return of getOverlayDataBuffer().
	 */
	/*   */ void saveOverlayData(const Str* data);
	/**
	 * @brief Overridable method dedicated to assembling the string of Overlay data.
	 */
	virtual Str* makeOverlayData();
	/**
	 * @brief Overridable method dedicated to controlling when/how the Overlay data is pushed to
	 *        the public-facing buffer, typically used along with saveOverlayData().
	 */
	virtual void pushOverlayData();

public:
	/**
	 * @brief Public-facing method dedicated to fetching a copy of the Overlay data string from
	 *        the public-facing buffer, thread-safe.
	 * @note This method should not be used across a DLL boundary, as it is not ABI-safe.
	 */
	Str copyOverlayData() const noexcept;
};
