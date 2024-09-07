/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <array>
#include <utility>
#include <execution>

#include "../../../Assistants/HomeDirManager.hpp"
#include "../../../Assistants/BasicVideoSpec.hpp"
#include "../../../Assistants/BasicAudioSpec.hpp"
#include "../../../Assistants/Well512.hpp"

#include "../HexInput.hpp"

#include "CHIP8_MODERN.hpp"

/*==================================================================*/

CHIP8_MODERN::CHIP8_MODERN() noexcept {
	if (getCoreState() != EmuState::FAILED) {

		copyGameToMemory(mMemoryBank.data(), cGameLoadPos);
		copyFontToMemory(mMemoryBank.data(), 0x0, 0x50);

		setDisplayResolution(cScreenSizeX, cScreenSizeY);

		BVS->setBackColor(cBitsColor[0]);
		BVS->createTexture(cScreenSizeX, cScreenSizeY);
		BVS->setAspectRatio(512, 256, +2);

		mProgCounter    = cStartOffset;
		mFramerate      = cRefreshRate;
		mCyclesPerFrame = Quirk.waitVblank ? cInstSpeedHi : cInstSpeedLo;
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
			if (Input->keyPressed(mRegisterV[mInputReg], mTotalFrames)) {
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
			return 0xFF000000 | cBitsColor[pixel >> 3];
		}
	);

	std::transform(
		std::execution::unseq,
		mDisplayBuffer.begin(),
		mDisplayBuffer.end(),
		mDisplayBuffer.begin(),
		[](const u32 pixel) noexcept {
			return static_cast<u8>(
				(pixel & 0x8) | (pixel >> 1)
			);
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

/*==================================================================*/


/*==================================================================*/
	#pragma region 0 instruction branch
/*==================================================================*/

	// 00E0 - erase whole display
	void CHIP8_MODERN::instruction_00E0() noexcept {
		if (Quirk.waitVblank) [[unlikely]]
			{ triggerInterrupt(Interrupt::FRAME); }
		std::fill(
			std::execution::unseq,
			mDisplayBuffer.begin(),
			mDisplayBuffer.end(),
			u8()
		);
	}
	// 00EE - return from subroutine
	void CHIP8_MODERN::instruction_00EE() noexcept {
		mProgCounter = mStackBank[--mStackTop & 0xF];
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 1 instruction branch
/*==================================================================*/

	// 1NNN - jump to NNN
	void CHIP8_MODERN::instruction_1NNN(const s32 NNN) noexcept {
		jumpProgramTo(NNN);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 2 instruction branch
/*==================================================================*/

	// 2NNN - call subroutine at NNN
	void CHIP8_MODERN::instruction_2NNN(const s32 NNN) noexcept {
		mStackBank[mStackTop++ & 0xF] = mProgCounter;
		jumpProgramTo(NNN);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 3 instruction branch
/*==================================================================*/

	// 3XNN - skip next instruction if VX == NN
	void CHIP8_MODERN::instruction_3xNN(const s32 X, const s32 NN) noexcept {
		if (mRegisterV[X] == NN) { nextInstruction(); }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 4 instruction branch
/*==================================================================*/

	// 4XNN - skip next instruction if VX != NN
	void CHIP8_MODERN::instruction_4xNN(const s32 X, const s32 NN) noexcept {
		if (mRegisterV[X] != NN) { nextInstruction(); }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 5 instruction branch
/*==================================================================*/

	// 5XY0 - skip next instruction if VX == VY
	void CHIP8_MODERN::instruction_5xy0(const s32 X, const s32 Y) noexcept {
		if (mRegisterV[X] == mRegisterV[Y]) { nextInstruction(); }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 6 instruction branch
/*==================================================================*/

	// 6XNN - set VX = NN
	void CHIP8_MODERN::instruction_6xNN(const s32 X, const s32 NN) noexcept {
		mRegisterV[X] = static_cast<u8>(NN);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 7 instruction branch
/*==================================================================*/

	// 7XNN - set VX = VX + NN
	void CHIP8_MODERN::instruction_7xNN(const s32 X, const s32 NN) noexcept {
		mRegisterV[X] += static_cast<u8>(NN);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 8 instruction branch
/*==================================================================*/

	// 8XY0 - set VX = VY
	void CHIP8_MODERN::instruction_8xy0(const s32 X, const s32 Y) noexcept {
		mRegisterV[X] = mRegisterV[Y];
	}
	// 8XY1 - set VX = VX | VY
	void CHIP8_MODERN::instruction_8xy1(const s32 X, const s32 Y) noexcept {
		mRegisterV[X] |= mRegisterV[Y];
	}
	// 8XY2 - set VX = VX & VY
	void CHIP8_MODERN::instruction_8xy2(const s32 X, const s32 Y) noexcept {
		mRegisterV[X] &= mRegisterV[Y];
	}
	// 8XY3 - set VX = VX ^ VY
	void CHIP8_MODERN::instruction_8xy3(const s32 X, const s32 Y) noexcept {
		mRegisterV[X] ^= mRegisterV[Y];
	}
	// 8XY4 - set VX = VX + VY, VF = carry
	void CHIP8_MODERN::instruction_8xy4(const s32 X, const s32 Y) noexcept {
		const auto sum{ mRegisterV[X] + mRegisterV[Y] };
		mRegisterV[X]   = static_cast<u8>(sum);
		mRegisterV[0xF] = static_cast<u8>(sum >> 8);
	}
	// 8XY5 - set VX = VX - VY, VF = !borrow
	void CHIP8_MODERN::instruction_8xy5(const s32 X, const s32 Y) noexcept {
		const bool nborrow{ mRegisterV[X] >= mRegisterV[Y] };
		mRegisterV[X]   = mRegisterV[X] - mRegisterV[Y];
		mRegisterV[0xF] = nborrow;
	}
	// 8XY7 - set VX = VY - VX, VF = !borrow
	void CHIP8_MODERN::instruction_8xy7(const s32 X, const s32 Y) noexcept {
		const bool nborrow{ mRegisterV[Y] >= mRegisterV[X] };
		mRegisterV[X]   = mRegisterV[Y] - mRegisterV[X];
		mRegisterV[0xF] = nborrow;
	}
	// 8XY6 - set VX = VY >> 1, VF = carry
	void CHIP8_MODERN::instruction_8xy6(const s32 X, const s32 Y) noexcept {
		if (!Quirk.shiftVX) { mRegisterV[X] = mRegisterV[Y]; }
		const bool lsb{ (mRegisterV[X] & 1) == 1 };
		mRegisterV[X]   = mRegisterV[X] >> 1;
		mRegisterV[0xF] = lsb;
	}
	// 8XYE - set VX = VY << 1, VF = carry
	void CHIP8_MODERN::instruction_8xyE(const s32 X, const s32 Y) noexcept {
		if (!Quirk.shiftVX) { mRegisterV[X] = mRegisterV[Y]; }
		const bool msb{ (mRegisterV[X] >> 7) == 1 };
		mRegisterV[X]   = mRegisterV[X] << 1;
		mRegisterV[0xF] = msb;
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 9 instruction branch
/*==================================================================*/

	// 9XY0 - skip next instruction if VX != VY
	void CHIP8_MODERN::instruction_9xy0(const s32 X, const s32 Y) noexcept {
		if (mRegisterV[X] != mRegisterV[Y]) { nextInstruction(); }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region A instruction branch
/*==================================================================*/

	// ANNN - set I = NNN
	void CHIP8_MODERN::instruction_ANNN(const s32 NNN) noexcept {
		mRegisterI = NNN & 0xFFF;
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region B instruction branch
/*==================================================================*/

	// BXNN - jump to NNN + V0
	void CHIP8_MODERN::instruction_BNNN(const s32 NNN) noexcept {
		jumpProgramTo(NNN + mRegisterV[0]);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region C instruction branch
/*==================================================================*/

	// CXNN - set VX = rnd(256) & NN
	void CHIP8_MODERN::instruction_CxNN(const s32 X, const s32 NN) noexcept {
		mRegisterV[X] = static_cast<u8>(Wrand->get() & NN);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region D instruction branch
/*==================================================================*/

	void CHIP8_MODERN::drawByte(s32 X, s32 Y, const u32 DATA) noexcept {
		switch (DATA) {
			[[unlikely]]
			case 0b00000000:
				return;

			[[likely]]
			case 0b10000000:
				if (Quirk.wrapSprite) { X &= mDisplayWb; }
				if (X < mDisplayW) {
					if (!((mDisplayBuffer[Y * mDisplayW + X] ^= 0x8) & 0x8))
						{ mRegisterV[0xF] = 1; }
				}
				return;

			[[unlikely]]
			default:
				if (Quirk.wrapSprite) { X &= mDisplayWb; }
				else if (X >= mDisplayW) { return; }

				for (auto B{ 0 }; B < 8; ++B, ++X &= mDisplayWb) {
					if (DATA & 0x80 >> B) {
						if (!((mDisplayBuffer[Y * mDisplayW + X] ^= 0x8) & 0x8))
							{ mRegisterV[0xF] = 1; }
					}
					if (!Quirk.wrapSprite && X == mDisplayWb) { return; }
				}
				return;
		}
	}

	// DXYN - draw N sprite rows at VX and VY
	void CHIP8_MODERN::instruction_DxyN(const s32 X, const s32 Y, const s32 N) noexcept {
		if (Quirk.waitVblank) [[unlikely]]
			{ triggerInterrupt(Interrupt::FRAME); }

		auto pX{ mRegisterV[X] & mDisplayWb };
		auto pY{ mRegisterV[Y] & mDisplayHb };

		mRegisterV[0xF] = 0;

		switch (N) {
			[[likely]]
			case 1:
				drawByte(pX, pY, readMemoryI(0));
				break;

			[[unlikely]]
			case 0:
				for (auto H{ 0 }, I{ 0 }; H < 16; ++H, I += 2, ++pY &= mDisplayHb)
				{
					drawByte(pX + 0, pY, readMemoryI(I + 0));
					drawByte(pX + 8, pY, readMemoryI(I + 1));
					
					if (!Quirk.wrapSprite && pY == mDisplayHb) { break; }
				}
				break;

			[[unlikely]]
			default:
				for (auto H{ 0 }; H < N; ++H, ++pY &= mDisplayHb)
				{
					drawByte(pX, pY, readMemoryI(H));
					if (!Quirk.wrapSprite && pY == mDisplayHb) { break; }
				}
				break;
		}
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region E instruction branch
/*==================================================================*/

	// EX9E - skip next instruction if key VX down (p1)
	void CHIP8_MODERN::instruction_Ex9E(const s32 X) noexcept {
		if ( Input->keyHeld_P1(mRegisterV[X])) { nextInstruction(); }
	}
	// EXA1 - skip next instruction if key VX up (p1)
	void CHIP8_MODERN::instruction_ExA1(const s32 X) noexcept {
		if (!Input->keyHeld_P1(mRegisterV[X])) { nextInstruction(); }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region F instruction branch
/*==================================================================*/

	// FX07 - set VX = delay timer
	void CHIP8_MODERN::instruction_Fx07(const s32 X) noexcept {
		mRegisterV[X] = mDelayTimer;
	}
	// FX0A - set VX = key, wait for keypress
	void CHIP8_MODERN::instruction_Fx0A(const s32 X) noexcept {
		triggerInterrupt(Interrupt::INPUT);
		mInputReg = static_cast<u8>(X);
	}
	// FX15 - set delay timer = VX
	void CHIP8_MODERN::instruction_Fx15(const s32 X) noexcept {
		mDelayTimer = mRegisterV[X];
	}
	// FX18 - set sound timer = VX
	void CHIP8_MODERN::instruction_Fx18(const s32 X) noexcept {
		mAudioTone  = calcAudioTone();
		mSoundTimer = mRegisterV[X] + (mRegisterV[X] == 1);
	}
	// FX1E - set I = I + VX
	void CHIP8_MODERN::instruction_Fx1E(const s32 X) noexcept {
		(mRegisterI += mRegisterV[X]) &= 0xFFF;
	}
	// FX29 - point I to 5 byte hex sprite from value in VX
	void CHIP8_MODERN::instruction_Fx29(const s32 X) noexcept {
		mRegisterI = (mRegisterV[X] & 0xF) * 5;
	}
	// FX33 - store BCD of VX to RAM at I, I+1, I+2
	void CHIP8_MODERN::instruction_Fx33(const s32 X) noexcept {
		writeMemoryI(mRegisterV[X] / 100,     0);
		writeMemoryI(mRegisterV[X] / 10 % 10, 1);
		writeMemoryI(mRegisterV[X]      % 10, 2);
	}
	// FX55 - store V0..VX to RAM at I..I+X
	void CHIP8_MODERN::instruction_Fx55(const s32 X) noexcept {
		for (auto idx{ 0 }; idx <= X; ++idx)
			{ writeMemoryI(mRegisterV[idx], idx); }
		if (!Quirk.idxRegNoInc) [[likely]]
			{ mRegisterI = mRegisterI + X + 1 & 0xFFF; }
	}
	// FX65 - load V0..VX from RAM at I..I+X
	void CHIP8_MODERN::instruction_Fx65(const s32 X) noexcept {
		for (auto idx{ 0 }; idx <= X; ++idx)
			{ mRegisterV[idx] = readMemoryI(idx); }
		if (!Quirk.idxRegNoInc) [[likely]]
			{ mRegisterI = mRegisterI + X + 1 & 0xFFF; }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

