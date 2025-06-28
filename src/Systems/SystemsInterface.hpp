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
#include <array>
#include <cmath>
#include <mutex>
#include <span>
#include <bit>

#include "../Assistants/AudioSpecBlock.hpp"
#include "../Assistants/Typedefs.hpp"
#include "../Assistants/Concepts.hpp"
#include "../Assistants/Misc.hpp"
#include "../Assistants/ColorOps.hpp"

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
class BasicAudioSpec;

class BasicKeyboard;
class FrameLimiter;
class Well512;

/*==================================================================*/

class SystemsInterface {
	Thread mCoreThread;

	Str mOverlayDataBuffer{};
	AtomSharedPtr<Str> mOverlayData;

protected:
	Str* getOverlayDataBuffer() noexcept
		{ return &mOverlayDataBuffer; }

protected:
	static inline HomeDirManager* HDM{};
	static inline BasicVideoSpec* BVS{};
	static inline Well512*        RNG{};

public:
	void startWorker() noexcept;
	void stopWorker() noexcept;

protected:
	void threadEntry(StopToken token);

	std::unique_ptr<FrameLimiter>  Pacer;
	std::unique_ptr<BasicKeyboard> Input;

	u64 mElapsedCycles{};
	Atom<f32> mTargetFPS{};
	s32 mTargetCPF{};

private:
	Atom<u32> mGlobalState{ EmuState::NORMAL };

protected:
	SystemsInterface() noexcept;

public:
	virtual ~SystemsInterface() noexcept;

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
	void setViewportSizes(bool cond, s32 W, s32 H, s32 mult, s32 ppad) noexcept;

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
