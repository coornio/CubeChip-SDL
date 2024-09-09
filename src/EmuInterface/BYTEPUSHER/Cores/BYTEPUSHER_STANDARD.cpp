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

BYTEPUSHER_STANDARD::BYTEPUSHER_STANDARD() noexcept
	: mMemoryBank(cTotalMemory + cSafezoneOOB)
{
	if (getSystemState() != EmuState::FAILED) {
		copyGameToMemory(mMemoryBank.data());

		BVS->setBackColor(cBitsColor[0]);
		BVS->setFrameColor(cBitsColor[0], cBitsColor[0]);
		BVS->createTexture(cScreenSizeX, cScreenSizeY);
		BVS->setAspectRatio(512, 512, -2);

		mCyclesPerFrame = 0x10000;
		mFramerate = cRefreshRate;
	}
}

/*==================================================================*/

void BYTEPUSHER_STANDARD::instructionLoop() noexcept {
	const auto inputStates{ getKeyStates() };
	      auto progPointer{ readData<3>(2) };
	mMemoryBank[0] = static_cast<u8>(inputStates >> 0x8);
	mMemoryBank[1] = static_cast<u8>(inputStates & 0xFF);
	
	auto cycleCount{ 0 };
	for (; cycleCount < mCyclesPerFrame; ++cycleCount) {
		mMemoryBank[readData<3>(progPointer + 3)] =
		mMemoryBank[readData<3>(progPointer + 0)];
		progPointer = readData<3>(progPointer + 6);
	}
	mTotalCycles += cycleCount;
}

void BYTEPUSHER_STANDARD::renderAudioData() {
	std::array<u8, cAudioLength> samplesBuffer{};

	const std::span<u8, cAudioLength>
		samplesOffset{ &mMemoryBank[readData<2>(6) << 8], cAudioLength };

	std::transform(
		std::execution::unseq,
		samplesOffset.begin(),
		samplesOffset.end(),
		samplesBuffer.data(),
		[volume = ASB->getVolumeNorm()](const s8 sample) noexcept {
			return static_cast<u8>(sample * volume);
		}
	);

	ASB->pushAudioData(std::span{ samplesBuffer });
}

void BYTEPUSHER_STANDARD::renderVideoData() {
	const std::span<u8, cScreenSizeX * cScreenSizeY>
		displayBuffer{ &mMemoryBank[readData<1>(5) << 16], cScreenSizeX * cScreenSizeY };

	BVS->modifyTexture<u8>(displayBuffer,
		[](const u32 pixel) noexcept {
			return 0xFF000000 | cBitsColor[pixel];
		}
	);
}
