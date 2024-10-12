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
	void loadCustomBinds(std::span<const SimpleKeyMapping> binds);

	u64  mTotalCycles{};
	u32  mTotalFrames{};

	u32  mCoreState{};
	f32  mFramerate{};
	s32  mActiveCPF{};

	void addCoreState(const EmuState state) noexcept { mCoreState |=  state; }
	void subCoreState(const EmuState state) noexcept { mCoreState &= ~state; }
	void xorCoreState(const EmuState state) noexcept { mCoreState ^=  state; }

	void setCoreState(const EmuState state) noexcept { mCoreState = state; }
	auto getCoreState()               const noexcept { return mCoreState;  }

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

	u32  getTotalFrames() const noexcept override { return mTotalFrames; }
	u64  getTotalCycles() const noexcept override { return mTotalCycles; }

	s32  getCPF()       const noexcept override { return mActiveCPF; }
	f32  getFramerate() const noexcept override { return mFramerate; }

	s32  changeCPF(const s32) noexcept override { return mActiveCPF; }

protected:

};
