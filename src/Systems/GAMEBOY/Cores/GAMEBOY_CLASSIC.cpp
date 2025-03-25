/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../../Assistants/BasicVideoSpec.hpp"
#include "../../../Assistants/BasicAudioSpec.hpp"
#include "../../../Assistants/Well512.hpp"
#include "../../CoreRegistry.hpp"

#include "GAMEBOY_CLASSIC.hpp"

static CoreRegistry::Register<GAMEBOY_CLASSIC> self(
	GAMEBOY_CLASSIC::validateProgram,
	{ ".gb" }
);

/*==================================================================*/

GAMEBOY_CLASSIC::GAMEBOY_CLASSIC() {
	BVS->setViewportSizes(cScreenSizeX, cScreenSizeY, cResSizeMult, +2);

	setSystemFramerate(cRefreshRate);

	mTargetCPF = cCylesPerSec;
}

/*==================================================================*/

void GAMEBOY_CLASSIC::instructionLoop() noexcept {
	const auto maxCycles{ static_cast<s32>(mTargetCPF / cRefreshRate) };

	auto curCycles{ 0 };
	while (curCycles < maxCycles) {
		
	}
}

void GAMEBOY_CLASSIC::renderAudioData() {
	
}

void GAMEBOY_CLASSIC::renderVideoData() {
	
}
