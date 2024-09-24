/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../../Assistants/HomeDirManager.hpp"
#include "../../../Assistants/BasicVideoSpec.hpp"
#include "../../../Assistants/BasicAudioSpec.hpp"
#include "../../../Assistants/Well512.hpp"

#include "GAMEBOY_CLASSIC.hpp"

/*==================================================================*/

GAMEBOY_CLASSIC::GAMEBOY_CLASSIC() {
	if (getSystemState() != EmuState::FAILED) {
		
		BVS->createTexture(cScreenSizeX, cScreenSizeY);
		BVS->setAspectRatio(cScreenSizeX * cResSizeMult, cScreenSizeY * cResSizeMult, +2);

		mActiveCPF = cCylesPerSec;
		mFramerate = cRefreshRate;
	}
}

/*==================================================================*/

void GAMEBOY_CLASSIC::instructionLoop() noexcept {
	const auto maxCycles{ static_cast<s32>(mActiveCPF / cRefreshRate) };

	auto curCycles{ 0 };
	while (curCycles < maxCycles) {
		
	}
}

void GAMEBOY_CLASSIC::renderAudioData() {
	
}

void GAMEBOY_CLASSIC::renderVideoData() {
	
}
