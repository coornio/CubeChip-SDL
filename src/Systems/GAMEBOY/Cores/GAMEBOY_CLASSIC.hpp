/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../GameBoy_CoreInterface.hpp"

/*==================================================================*/

class GAMEBOY_CLASSIC final : public GameBoy_CoreInterface {
	static constexpr u32 cTotalMemory{    0x2000 };
	static constexpr u32 cSafezoneOOB{         8 };
	static constexpr f32 cRefreshRate{ 59.72750f };
	static constexpr s32 cAudioLength{       256 };
	static constexpr s32 cScreenSizeX{       160 };
	static constexpr s32 cScreenSizeY{       144 };
	static constexpr s32 cScreenSizeT{ 160 * 144 };

private:
	void instructionLoop() noexcept override;
	void renderAudioData() override;
	void renderVideoData() override;

public:
	GAMEBOY_CLASSIC();

	static constexpr bool testGameSize(const usz size) noexcept {
		return size <= cTotalMemory;
	}
};
