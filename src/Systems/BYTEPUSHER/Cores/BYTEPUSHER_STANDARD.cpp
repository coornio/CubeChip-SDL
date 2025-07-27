/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BYTEPUSHER_STANDARD.hpp"
#if defined(ENABLE_BYTEPUSHER_STANDARD) && defined(ENABLE_BYTEPUSHER_STANDARD)

#include "../../../Assistants/BasicVideoSpec.hpp"
#include "../../../Assistants/GlobalAudioBase.hpp"
#include "../../CoreRegistry.hpp"

REGISTER_CORE(BYTEPUSHER_STANDARD, ".BytePusher")

/*==================================================================*/

BYTEPUSHER_STANDARD::BYTEPUSHER_STANDARD() {
	copyGameToMemory(mMemoryBank.data());

	setDisplayBorderColor(cBitsColor[0]);

	setViewportSizes(true, cScreenSizeX, cScreenSizeY, cResSizeMult, 2);
	setSystemFramerate(cRefreshRate);

	mAudioDevice.addAudioStream(STREAM::MAIN, u32(cRefreshRate * cAudioLength));
	mAudioDevice.resumeStreams();
}

/*==================================================================*/

void BYTEPUSHER_STANDARD::instructionLoop() noexcept {
	const auto inputStates{ getKeyStates() };
	      auto progPointer{ readData<3>(2) };

	::assign_cast(mMemoryBank[0], inputStates >> 0x8);
	::assign_cast(mMemoryBank[1], inputStates & 0xFF);
	
	auto cycleCount{ 0 };
	for (; cycleCount < 0x10000; ++cycleCount) {
		mMemoryBank[readData<3>(progPointer + 3)] =
		mMemoryBank[readData<3>(progPointer + 0)];
		progPointer = readData<3>(progPointer + 6);
	}
}

void BYTEPUSHER_STANDARD::renderAudioData() {
	const auto samplesOffset{ mMemoryBank.data() + (readData<2>(6) << 8) };
	auto buffer{ ::allocate_n<f32>(cAudioLength).as_value().release_as_container() };

	static constexpr auto master_gain{ 0.22f };

	std::transform(EXEC_POLICY(unseq)
		samplesOffset, samplesOffset + cAudioLength, buffer.data(),
		[](const auto sample) noexcept
			{ return s8(sample) * (master_gain / 127.0f); }
	);

	mAudioDevice[STREAM::MAIN].pushAudioData(buffer);
}

void BYTEPUSHER_STANDARD::renderVideoData() {
	BVS->displayBuffer.write(mMemoryBank.data() + (readData<1>(5) << 16), cScreenSizeX * cScreenSizeY,
		[](u32 pixel) noexcept { return cBitsColor[pixel]; });
}

#endif
