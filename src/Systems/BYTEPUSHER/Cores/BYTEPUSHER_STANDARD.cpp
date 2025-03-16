/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../../Assistants/BasicVideoSpec.hpp"
#include "../../../Assistants/BasicAudioSpec.hpp"

#include "BYTEPUSHER_STANDARD.hpp"

/*==================================================================*/

BYTEPUSHER_STANDARD::BYTEPUSHER_STANDARD() {
	copyGameToMemory(mMemoryBank.data());

	BVS->setOutlineColor(cBitsColor[0]);
	BVS->setViewportSizes(cScreenSizeX, cScreenSizeY, cResSizeMult, -2);

	setSystemFramerate(cRefreshRate);

	mTargetCPF.store(0x10000, mo::release);
}

/*==================================================================*/

void BYTEPUSHER_STANDARD::instructionLoop() noexcept {
	const auto inputStates{ getKeyStates() };
	      auto progPointer{ readData<3>(2) };

	mMemoryBank[0] = static_cast<u8>(inputStates >> 0x8);
	mMemoryBank[1] = static_cast<u8>(inputStates & 0xFF);
	
	auto cycleCount{ 0 };
	for (; cycleCount < mTargetCPF.load(mo::acquire); ++cycleCount) {
		mMemoryBank[readData<3>(progPointer + 3)] =
		mMemoryBank[readData<3>(progPointer + 0)];
		progPointer = readData<3>(progPointer + 6);
	}
	mElapsedCycles.fetch_add(cycleCount, mo::acq_rel);
}

void BYTEPUSHER_STANDARD::renderAudioData() {
	const auto samplesOffset{ mMemoryBank.data() + (readData<2>(6) << 8) };
	std::vector<s16> samplesBuffer(cAudioLength);

	std::transform(EXEC_POLICY(unseq)
		samplesOffset,
		samplesOffset + cAudioLength,
		samplesBuffer.data(),
		[](const u8 sample) noexcept {
			return static_cast<s16>(sample << 8);
		}
	);

	ASB->pushAudioData(STREAM::CHANN0, samplesBuffer);
}

void BYTEPUSHER_STANDARD::renderVideoData() {
	BVS->displayBuffer.write(mMemoryBank.data() + (readData<1>(5) << 16), cScreenSizeT,
		[](u32 pixel) noexcept { return cBitsColor[pixel]; });
}
