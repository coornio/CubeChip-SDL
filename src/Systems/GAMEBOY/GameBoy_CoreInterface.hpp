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

	AudioSpecBlock mAudio;

	std::vector<SimpleKeyMapping> mCustomBinds;

	u32  getKeyStates() const;
	void loadPresetBinds();

	template <IsContiguousContainer T> requires
		SameValueTypes<T, decltype(mCustomBinds)>
	void loadCustomBinds(const T& binds) {
		mCustomBinds.assign(std::begin(binds), std::end(binds));
	}

	void copyGameToMemory(u8* dest) noexcept;

	virtual void updateKeyStates() noexcept = 0;
	virtual void instructionLoop() noexcept = 0;
	virtual void renderAudioData() = 0;
	virtual void renderVideoData() = 0;

protected:
	GameBoy_CoreInterface() noexcept;

public:
	void mainSystemLoop() override;
};
