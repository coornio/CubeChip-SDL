/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../EmuInterface.hpp"

/*==================================================================*/

class GameBoy_CoreInterface : public EmuInterface {

protected:
	static inline Path* sSavestatePath{};

	std::unique_ptr<AudioSpecBlock> ASB;

	std::vector<SimpleKeyMapping> mCustomBinds;

	u32  getKeyStates() const;
	void loadPresetBinds();

	template <IsContiguousContainer T> requires
		SameValueTypes<T, decltype(mCustomBinds)>
	void loadCustomBinds(const T& binds) {
		mCustomBinds.assign(std::begin(binds), std::end(binds));
	}

	u32  mCoreState{};

	void addCoreState(EmuState state) noexcept { mCoreState |=  state; }
	void subCoreState(EmuState state) noexcept { mCoreState &= ~state; }
	void xorCoreState(EmuState state) noexcept { mCoreState ^=  state; }

	void setCoreState(EmuState state) noexcept { mCoreState = state; }
	auto getCoreState()         const noexcept { return mCoreState;  }

	bool isSystemStopped() const noexcept override { return getCoreState() || getSystemState(); }
	bool isCoreStopped()   const noexcept override { return getCoreState(); }

	void copyGameToMemory(u8* dest) noexcept;

	virtual void updateKeyStates() noexcept = 0;
	virtual void instructionLoop() noexcept = 0;
	virtual void renderAudioData() = 0;
	virtual void renderVideoData() = 0;

public:
	GameBoy_CoreInterface() noexcept;
	~GameBoy_CoreInterface() noexcept override;

	void processFrame() override;

protected:
	s32  getCPF() const noexcept override { return mTargetCPF; }
	s32  addCPF(s32)    noexcept override { return mTargetCPF; }
};
