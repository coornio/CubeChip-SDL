/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "CHIP8_MODERN.hpp"

#include "../../HostClass/HomeDirManager.hpp"
#include "../../HostClass/BasicVideoSpec.hpp"
#include "../../HostClass/BasicAudioSpec.hpp"

/*==================================================================*/

CHIP8_MODERN::CHIP8_MODERN() noexcept {
	if (getCoreState() != EmuState::FAILED) {

		copyGameToMemory(mMemoryBank.data(), cGameLoadPos);
		copyFontToMemory(mMemoryBank.data(), 0x0, 0x50);

		mProgCounter    = cStartOffset;
		mFramerate      = cRefreshRate;
		mCyclesPerFrame = Quirk.waitVblank ? cInstSpeedHi : 6000000;

		setDisplayResolution(cScreenSizeX, cScreenSizeY);

		BVS->setBackColor(cBitsColor[0]);
		BVS->createTexture(cScreenSizeX, cScreenSizeY);
		BVS->setAspectRatio(512, 256, +2);
	}
}

/*==================================================================*/

void CHIP8_MODERN::handlePreFrameInterrupt() noexcept {
	switch (mInterruptType)
	{
		case Interrupt::FRAME:
			mInterruptType  = Interrupt::CLEAR;
			mCyclesPerFrame = std::abs(mCyclesPerFrame);
			return;

		case Interrupt::SOUND:
			if (!mSoundTimer) {
				mInterruptType = Interrupt::FINAL;
				mCyclesPerFrame = 0;
			}
			return;
	}
}

void CHIP8_MODERN::handleEndFrameInterrupt() noexcept {
	switch (mInterruptType)
	{
		case Interrupt::INPUT:
			if (Input.keyPressed(mRegisterV[mInputReg], mTotalFrames)) {
				mInterruptType  = Interrupt::CLEAR;
				mCyclesPerFrame = std::abs(mCyclesPerFrame);
				mAudioTone      = calcAudioTone();
				mSoundTimer     = 2;
			}
			return;

		case Interrupt::ERROR:
		case Interrupt::FINAL:
			setCoreState(EmuState::HALTED);
			mCyclesPerFrame = 0;
			return;
	}
}

void CHIP8_MODERN::handleTimerTick() noexcept {
	if (mDelayTimer) { --mDelayTimer; }
	if (mSoundTimer) { --mSoundTimer; }
}

void CHIP8_MODERN::instructionLoop() noexcept {

	auto cycleCount{ 0 };
	for (; cycleCount < mCyclesPerFrame; ++cycleCount) {
		const auto HI{ mMemoryBank[mProgCounter + 0u] };
		const auto LO{ mMemoryBank[mProgCounter + 1u] };
		nextInstruction();

		switch (HI >> 4) {
			case 0x0:
				switch (HI << 8 | LO) {
					case 0x00E0:
						instruction_00E0();
						break;
					case 0x00EE:
						instruction_00EE();
						break;
					[[unlikely]]
					default: instructionError(HI, LO);
				}
				break;
			case 0x1:
				instruction_1NNN(HI << 8 | LO);
				break;
			case 0x2:
				instruction_2NNN(HI << 8 | LO);
				break;
			case 0x3:
				instruction_3xNN(HI & 0xF, LO);
				break;
			case 0x4:
				instruction_4xNN(HI & 0xF, LO);
				break;
			case 0x5:
				if (LO & 0xF) [[unlikely]] {
					instructionError(HI, LO);
				} else {
					instruction_5xy0(HI & 0xF, LO >> 4);
				}
				break;
			case 0x6:
				instruction_6xNN(HI & 0xF, LO);
				break;
			case 0x7:
				instruction_7xNN(HI & 0xF, LO);
				break;
			case 0x8:
				switch (LO & 0xF) {
					case 0x0:
						instruction_8xy0(HI & 0xF, LO >> 4);
						break;
					case 0x1:
						instruction_8xy1(HI & 0xF, LO >> 4);
						break;
					case 0x2:
						instruction_8xy2(HI & 0xF, LO >> 4);
						break;
					case 0x3:
						instruction_8xy3(HI & 0xF, LO >> 4);
						break;
					case 0x4:
						instruction_8xy4(HI & 0xF, LO >> 4);
						break;
					case 0x5:
						instruction_8xy5(HI & 0xF, LO >> 4);
						break;
					case 0x7:
						instruction_8xy7(HI & 0xF, LO >> 4);
						break;
					case 0x6:
						instruction_8xy6(HI & 0xF, LO >> 4);
						break;
					case 0xE:
						instruction_8xyE(HI & 0xF, LO >> 4);
						break;
					[[unlikely]]
					default: instructionError(HI, LO);
				}
				break;
			case 0x9:
				if (LO & 0xF) [[unlikely]] {
					instructionError(HI, LO);
				} else {
					instruction_9xy0(HI & 0xF, LO >> 4);
				}
				break;
			case 0xA:
				instruction_ANNN(HI << 8 | LO);
				break;
			case 0xB:
				instruction_BNNN(HI << 8 | LO);
				break;
			case 0xC:
				instruction_CxNN(HI & 0xF, LO);
				break;
			[[likely]]
			case 0xD:
				instruction_DxyN(HI & 0xF, LO >> 4, LO & 0xF);
				break;
			case 0xE:
				switch (LO) {
					case 0x9E:
						instruction_Ex9E(HI & 0xF);
						break;
					case 0xA1:
						instruction_ExA1(HI & 0xF);
						break;
					[[unlikely]]
					default: instructionError(HI, LO);
				}
				break;
			case 0xF:
				switch (LO) {
					case 0x07:
						instruction_Fx07(HI & 0xF);
						break;
					case 0x0A:
						instruction_Fx0A(HI & 0xF);
						break;
					case 0x15:
						instruction_Fx15(HI & 0xF);
						break;
					case 0x18:
						instruction_Fx18(HI & 0xF);
						break;
					case 0x1E:
						instruction_Fx1E(HI & 0xF);
						break;
					case 0x29:
						instruction_Fx29(HI & 0xF);
						break;
					case 0x33:
						instruction_Fx33(HI & 0xF);
						break;
					case 0x55:
						instruction_Fx55(HI & 0xF);
						break;
					case 0x65:
						instruction_Fx65(HI & 0xF);
						break;
					[[unlikely]]
					default: instructionError(HI, LO);
				}
				break;
		}
	}
	mTotalCycles += cycleCount;
}

void CHIP8_MODERN::renderAudioData() {
	std::vector<s16> audioBuffer(static_cast<usz>(BAS->getFrequency() / cRefreshRate));

	if (mSoundTimer) {
		const auto amplitute{ BAS->getAmplitude() };
		for (auto& sample_s16 : audioBuffer) {
			sample_s16 = mWavePhase > 0.5f ? amplitute : -amplitute;
			mWavePhase = std::fmod(mWavePhase + mAudioTone, 1.0f);
		}
		BVS->setFrameColor(cBitsColor[0], cBitsColor[1]);
	} else {
		mWavePhase = 0.0f;
		BVS->setFrameColor(cBitsColor[0], cBitsColor[0]);
	}
	BAS->pushAudioData(audioBuffer.data(), audioBuffer.size());
}

void CHIP8_MODERN::renderVideoData() {
	BVS->modifyTexture<u8>(mDisplayBuffer, isPixelTrailing()
		? [](const u32 pixel) noexcept {
			static constexpr u32 layer[4]{ 0xFF, 0xE7, 0x6F, 0x37 };
			const auto alpha{ layer[std::countl_zero(pixel) & 0x3] };
			return alpha << 24 | cBitsColor[pixel != 0];
		}
		: [](const u32 pixel) noexcept {
			return 0xFF000000 | cBitsColor[pixel != 0];
		}
	);

	//std::array<u32, cScreenSizeX * cScreenSizeY>
	//	pixelColorData;
	//
	//std::transform(
	//	std::execution::unseq,
	//	mDisplayBuffer.begin(),
	//	mDisplayBuffer.end(),
	//	pixelColorData.begin(),
	//	isPixelTrailing()
	//	? [](const u32 pixel) noexcept {
	//		static constexpr u32 layer[4]{ 0xFF, 0xE7, 0x6F, 0x37 };
	//		const auto alpha{ layer[std::countl_zero(pixel) & 0x3] };
	//		return alpha << 24 | cBitsColor[pixel != 0];
	//	}
	//	: [](const u32 pixel) noexcept {
	//		return 0xFF000000 | cBitsColor[pixel != 0];
	//	}
	//);
	//BVS->modifyTexture(pixelColorData);

	std::transform(
		std::execution::unseq,
		mDisplayBuffer.begin(),
		mDisplayBuffer.end(),
		mDisplayBuffer.begin(),
		[](const u32 pixel) noexcept {
			return (pixel & 0x8) | (pixel >> 1);
		}
	);
}

/*==================================================================*/

f32  CHIP8_MODERN::calcAudioTone() const noexcept {
	return (160.0f + 8.0f * (
		(mProgCounter >> 1) + mStackTop + 1 & 0x3E
	)) / BAS->getFrequency();
}

void CHIP8_MODERN::nextInstruction() noexcept {
	mProgCounter += 2;
}

void CHIP8_MODERN::jumpProgramTo(const u32 next) noexcept {
	const auto NNN{ next & 0xFFF };
	if (mProgCounter - 2u != NNN) [[likely]] {
		mProgCounter = NNN & 0xFFF;
	} else {
		triggerInterrupt(Interrupt::SOUND);
	}
}
