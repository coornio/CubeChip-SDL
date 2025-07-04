/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BYTEPUSHER_STANDARD.hpp"
#ifdef ENABLE_BYTEPUSHER_STANDARD

#include "../../../Assistants/BasicVideoSpec.hpp"
#include "../../../Assistants/BasicAudioSpec.hpp"
#include "../../CoreRegistry.hpp"

REGISTER_CORE(BYTEPUSHER_STANDARD, ".BytePusher")

/*==================================================================*/

BYTEPUSHER_STANDARD::BYTEPUSHER_STANDARD() {
	copyGameToMemory(mMemoryBank.data());

	setDisplayBorderColor(cBitsColor[0]);

	setViewportSizes(true, cScreenSizeX, cScreenSizeY, cResSizeMult, 2);
	setSystemFramerate(cRefreshRate);
}

/*==================================================================*/

void BYTEPUSHER_STANDARD::instructionLoop() noexcept {
	const auto inputStates{ getKeyStates() };
	      auto progPointer{ readData<3>(2) };

	mMemoryBank[0] = static_cast<u8>(inputStates >> 0x8);
	mMemoryBank[1] = static_cast<u8>(inputStates & 0xFF);
	
	auto cycleCount{ 0 };
	for (; cycleCount < 0x10000; ++cycleCount) {
		mMemoryBank[readData<3>(progPointer + 3)] =
		mMemoryBank[readData<3>(progPointer + 0)];
		progPointer = readData<3>(progPointer + 6);
	}
	mElapsedCycles += cycleCount;
}

void BYTEPUSHER_STANDARD::renderAudioData() {
	const auto samplesOffset{ mMemoryBank.data() + (readData<2>(6) << 8) };
	auto samplesBuffer{ ::allocate<s16>(cAudioLength).as_value().release() };

	std::transform(EXEC_POLICY(unseq)
		samplesOffset, samplesOffset + cAudioLength, samplesBuffer.get(),
		[](const auto sample) noexcept
			{ return static_cast<s16>(sample << 8); }
	);

	mAudio[STREAM::CHANN0].pushAudioData(samplesBuffer.get(), cAudioLength);
}

void BYTEPUSHER_STANDARD::renderVideoData() {
	BVS->displayBuffer.write(mMemoryBank.data() + (readData<1>(5) << 16), cScreenSizeX * cScreenSizeY,
		[](u32 pixel) noexcept { return cBitsColor[pixel]; });
}

#endif
