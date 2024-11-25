/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../../Assistants/HomeDirManager.hpp"
#include "../../../Assistants/BasicVideoSpec.hpp"
#include "../../../Assistants/BasicAudioSpec.hpp"
#include "../../../Assistants/Well512.hpp"

#include "BYTEPUSHER_STANDARD.hpp"

/*==================================================================*/

BYTEPUSHER_STANDARD::BYTEPUSHER_STANDARD() {
	if (getSystemState() != EmuState::FATAL) {
		copyGameToMemory(mMemoryBank.data());

		BVS->setFrameColor(cBitsColor[0], cBitsColor[0]);
		if (!BVS->setViewportDimensions(cScreenSizeX, cScreenSizeY, 1, -2))
			[[unlikely]] { addCoreState(EmuState::FATAL); }

		mActiveCPF = 0x10000;
		mFramerate = cRefreshRate;
	}
}

/*==================================================================*/

void BYTEPUSHER_STANDARD::instructionLoop() noexcept {
	const auto inputStates{ getKeyStates() };
	      auto progPointer{ readData<3>(2) };

	mMemoryBank[0] = static_cast<u8>(inputStates >> 0x8);
	mMemoryBank[1] = static_cast<u8>(inputStates & 0xFF);
	
	for (auto cycleCount{ 0 }; cycleCount < mActiveCPF; ++cycleCount) {
		mMemoryBank[readData<3>(progPointer + 3)] =
		mMemoryBank[readData<3>(progPointer + 0)];
		progPointer = readData<3>(progPointer + 6);
	}
	mTotalCycles += mActiveCPF;
}

void BYTEPUSHER_STANDARD::renderAudioData() {
	const auto samplesOffset{ mMemoryBank.data() + (readData<2>(6) << 8) };

	std::vector<s8> samplesBuffer \
		(samplesOffset, samplesOffset + cAudioLength);

	ASB->pushAudioData(STREAM::CHANN0, samplesBuffer);
}

void BYTEPUSHER_STANDARD::renderVideoData() {
	const auto displayOffset{ mMemoryBank.data() + (readData<1>(5) << 16) };

	const std::span<const u8, cScreenSizeT>
		displayBuffer{ displayOffset, cScreenSizeT };

	BVS->modifyTexture(displayBuffer,
		[](const u32 pixel) noexcept {
			return 0xFF000000 | cBitsColor[pixel];
		}
	);
}
