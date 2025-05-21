/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../../Assistants/BasicVideoSpec.hpp"
#include "../../../Assistants/BasicAudioSpec.hpp"
#include "../../../Assistants/Well512.hpp"
#include "../../CoreRegistry.hpp"

#include "CHIP8X.hpp"

REGISTER_CORE(CHIP8X, ".c8x")

/*==================================================================*/

CHIP8X::CHIP8X() {
	std::fill(EXEC_POLICY(unseq)
		mMemoryBank.end() - cSafezoneOOB, mMemoryBank.end(), u8{ 0xFF });

	copyGameToMemory(mMemoryBank.data() + cGameLoadPos);
	copyFontToMemory(mMemoryBank.data(), 0x50);

	setDisplayResolution(cScreenSizeX, cScreenSizeY);
	setViewportSizes(true, cScreenSizeX, cScreenSizeY, cResSizeMult, 2);
	setSystemFramerate(cRefreshRate);

	mCurrentPC = cStartOffset;
	mTargetCPF = cInstSpeedHi;

	mColoredBuffer(0, 0) = cForeColor[2];

	mAudio.pauseStream(STREAM::CHANN1);
	mAudio.pauseStream(STREAM::CHANN2);
}

/*==================================================================*/

void CHIP8X::instructionLoop() noexcept {

	auto cycleCount{ 0 };
	for (; cycleCount < mTargetCPF; ++cycleCount) {
		const auto HI{ mMemoryBank[mCurrentPC + 0u] };
		const auto LO{ mMemoryBank[mCurrentPC + 1u] };
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
					case 0x2A0:
						instruction_02A0();
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
				switch (LO & 0xF) {
					case 0:
						instruction_5xy0(HI & 0xF, LO >> 4);
						break;
					case 1:
						instruction_5xy1(HI & 0xF, LO >> 4);
						break;
						[[unlikely]]
					default: instructionError(HI, LO);
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
				if (HI == 0xBF) [[unlikely]] {
					instructionError(HI, LO);
				} else {
					instruction_BxyN(HI & 0xF, LO >> 4, LO & 0xF);
				}
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
					case 0xF2:
						instruction_ExF2(HI & 0xF);
						break;
					case 0xF5:
						instruction_ExF5(HI & 0xF);
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
						instruction_FN55(HI & 0xF);
						break;
					case 0x65:
						instruction_FN65(HI & 0xF);
						break;
					case 0xF8:
						instruction_FxF8(HI & 0xF);
						break;
					case 0xFB:
						instruction_FxFB(HI & 0xF);
						break;
						[[unlikely]]
					default: instructionError(HI, LO);
				}
				break;
		}
	}
	mElapsedCycles += cycleCount;
}

void CHIP8X::renderAudioData() {
	pushSquareTone(STREAM::UNIQUE);
	pushSquareTone(STREAM::BUZZER);

	static constexpr u32 idx[]{ 2, 7, 4, 1 };
	setDisplayBorderColor(
		std::accumulate(mAudioTimer.begin(), mAudioTimer.end(), 0)
		? cForeColor[idx[mBackgroundColor]] : cBackColor[mBackgroundColor]);
}

void CHIP8X::renderVideoData() {
	if (isUsingPixelTrails()) {
		BVS->displayBuffer.write(mDisplayBuffer, [this](const u8& pixel) noexcept {
			static constexpr u32 layer[4]{ 0xFF, 0xE7, 0x6F, 0x37 };
			const auto opacity{ layer[std::countl_zero(pixel) & 0x3] };
			const auto idx{ &pixel - mDisplayBuffer.data() };
			const auto Y = (idx / mDisplayW) & mColorResolution;
			const auto X = (idx % mDisplayW) >> 3;
			return (pixel != 0)
				? opacity | mColoredBuffer(X, Y)
				: 0xFF    | cBackColor[mBackgroundColor];
		});
	} else {
		BVS->displayBuffer.write(mDisplayBuffer, [this](const u8& pixel) noexcept {
			const auto idx{ &pixel - mDisplayBuffer.data() };
			const auto Y = (idx / mDisplayW) & mColorResolution;
			const auto X = (idx % mDisplayW) >> 3;
			return (pixel >> 3)
				? 0xFF | mColoredBuffer(X, Y)
				: 0xFF | cBackColor[mBackgroundColor];
		});
	}

	std::for_each(EXEC_POLICY(unseq)
		mDisplayBuffer.begin(),
		mDisplayBuffer.end(),
		[](auto& pixel) noexcept
			{ ::assign_cast(pixel, (pixel & 0x8) | (pixel >> 1)); }
	);
}

void CHIP8X::setBuzzerPitch(s32 pitch) noexcept {
	mPhaseStep[STREAM::UNIQUE] = (sTonalOffset + (
		(0xFF - (pitch ? pitch : 0x80)) >> 3 << 4)
	) / mAudio.getFrequency();
}

void CHIP8X::drawLoresColor(s32 X, s32 Y, s32 idx) noexcept {
	for (auto pY{ 0 }, maxH{ Y >> 4 }; pY <= maxH; ++pY) {
		for (auto pX{ 0 }, maxW{ X >> 4 }; pX <= maxW; ++pX) {
			mColoredBuffer((X + pX) & 0x7, ((Y + pY) << 2) & 0x1F) = cForeColor[idx & 0x7];
		}
	}
	mColorResolution = 0xFC;
}

void CHIP8X::drawHiresColor(s32 X, s32 Y, s32 idx, s32 N) noexcept {
	for (auto pY{ Y }, pX{ X >> 3 }; pY < Y + N; ++pY) {
		mColoredBuffer(pX & 0x7, pY & 0x1F) = cForeColor[idx & 0x7];
	}
	mColorResolution = 0xFF;
}

/*==================================================================*/
	#pragma region 0 instruction branch

	void CHIP8X::instruction_00E0() noexcept {
		triggerInterrupt(Interrupt::FRAME);
		initialize(mDisplayBuffer);
	}
	void CHIP8X::instruction_00EE() noexcept {
		mCurrentPC = mStackBank[--mStackTop & 0xF];
	}
	void CHIP8X::instruction_02A0() noexcept {
		setDisplayBorderColor(cBackColor[++mBackgroundColor &= 0x3]);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 1 instruction branch

	void CHIP8X::instruction_1NNN(s32 NNN) noexcept {
		performProgJump(NNN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 2 instruction branch

	void CHIP8X::instruction_2NNN(s32 NNN) noexcept {
		mStackBank[mStackTop++ & 0xF] = mCurrentPC;
		performProgJump(NNN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 3 instruction branch

	void CHIP8X::instruction_3xNN(s32 X, s32 NN) noexcept {
		if (mRegisterV[X] == NN) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 4 instruction branch

	void CHIP8X::instruction_4xNN(s32 X, s32 NN) noexcept {
		if (mRegisterV[X] != NN) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 5 instruction branch

	void CHIP8X::instruction_5xy0(s32 X, s32 Y) noexcept {
		if (mRegisterV[X] == mRegisterV[Y]) { skipInstruction(); }
	}
	void CHIP8X::instruction_5xy1(s32 X, s32 Y) noexcept {
		const auto lenX{ (mRegisterV[X] & 0x70) + (mRegisterV[Y] & 0x70) };
		const auto lenY{ (mRegisterV[X] + mRegisterV[Y]) & 0x7 };
		mRegisterV[X] = static_cast<u8>(lenX | lenY);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 6 instruction branch

	void CHIP8X::instruction_6xNN(s32 X, s32 NN) noexcept {
		mRegisterV[X] = static_cast<u8>(NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 7 instruction branch

	void CHIP8X::instruction_7xNN(s32 X, s32 NN) noexcept {
		mRegisterV[X] += static_cast<u8>(NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 8 instruction branch

	void CHIP8X::instruction_8xy0(s32 X, s32 Y) noexcept {
		mRegisterV[X] = mRegisterV[Y];
	}
	void CHIP8X::instruction_8xy1(s32 X, s32 Y) noexcept {
		mRegisterV[X] |= mRegisterV[Y];
	}
	void CHIP8X::instruction_8xy2(s32 X, s32 Y) noexcept {
		mRegisterV[X] &= mRegisterV[Y];
	}
	void CHIP8X::instruction_8xy3(s32 X, s32 Y) noexcept {
		mRegisterV[X] ^= mRegisterV[Y];
	}
	void CHIP8X::instruction_8xy4(s32 X, s32 Y) noexcept {
		const auto sum{ mRegisterV[X] + mRegisterV[Y] };
		::assign_cast(mRegisterV[X], sum);
		::assign_cast(mRegisterV[0xF], sum >> 8);
	}
	void CHIP8X::instruction_8xy5(s32 X, s32 Y) noexcept {
		const bool nborrow{ mRegisterV[X] >= mRegisterV[Y] };
		::assign_cast(mRegisterV[X], mRegisterV[X] - mRegisterV[Y]);
		::assign_cast(mRegisterV[0xF], nborrow);
	}
	void CHIP8X::instruction_8xy7(s32 X, s32 Y) noexcept {
		const bool nborrow{ mRegisterV[Y] >= mRegisterV[X] };
		::assign_cast(mRegisterV[X], mRegisterV[Y] - mRegisterV[X]);
		::assign_cast(mRegisterV[0xF], nborrow);
	}
	void CHIP8X::instruction_8xy6(s32 X, s32 Y) noexcept {
		if (!Quirk.shiftVX) { mRegisterV[X] = mRegisterV[Y]; }
		const bool lsb{ (mRegisterV[X] & 1) == 1 };
		::assign_cast(mRegisterV[X], mRegisterV[X] >> 1);
		::assign_cast(mRegisterV[0xF], lsb);
	}
	void CHIP8X::instruction_8xyE(s32 X, s32 Y) noexcept {
		if (!Quirk.shiftVX) { mRegisterV[X] = mRegisterV[Y]; }
		const bool msb{ (mRegisterV[X] >> 7) == 1 };
		::assign_cast(mRegisterV[X], mRegisterV[X] << 1);
		::assign_cast(mRegisterV[0xF], msb);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 9 instruction branch

	void CHIP8X::instruction_9xy0(s32 X, s32 Y) noexcept {
		if (mRegisterV[X] != mRegisterV[Y]) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region A instruction branch

	void CHIP8X::instruction_ANNN(s32 NNN) noexcept {
		mRegisterI = NNN & 0xFFF;
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region B instruction branch

	void CHIP8X::instruction_BxyN(s32 X, s32 Y, s32 N) noexcept {
		if (N) {
			drawHiresColor(mRegisterV[X], mRegisterV[X + 1], mRegisterV[Y] & 0x7, N);
		} else {
			drawLoresColor(mRegisterV[X], mRegisterV[X + 1], mRegisterV[Y] & 0x7);
		}
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region C instruction branch

	void CHIP8X::instruction_CxNN(s32 X, s32 NN) noexcept {
		mRegisterV[X] = RNG->next<u8>() & NN;
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region D instruction branch

	void CHIP8X::drawByte(s32 X, s32 Y, u32 DATA) noexcept {
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

	void CHIP8X::instruction_DxyN(s32 X, s32 Y, s32 N) noexcept {
		triggerInterrupt(Interrupt::FRAME);

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

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region E instruction branch

	void CHIP8X::instruction_Ex9E(s32 X) noexcept {
		if (keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
	}
	void CHIP8X::instruction_ExA1(s32 X) noexcept {
		if (!keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
	}
	void CHIP8X::instruction_ExF2(s32 X) noexcept {
		if (keyHeld_P2(mRegisterV[X])) { skipInstruction(); }
	}
	void CHIP8X::instruction_ExF5(s32 X) noexcept {
		if (!keyHeld_P2(mRegisterV[X])) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region F instruction branch

	void CHIP8X::instruction_Fx07(s32 X) noexcept {
		mRegisterV[X] = static_cast<u8>(mDelayTimer);
	}
	void CHIP8X::instruction_Fx0A(s32 X) noexcept {
		triggerInterrupt(Interrupt::INPUT);
		mInputReg = &mRegisterV[X];
	}
	void CHIP8X::instruction_Fx15(s32 X) noexcept {
		mDelayTimer = mRegisterV[X];
	}
	void CHIP8X::instruction_Fx18(s32 X) noexcept {
		mAudioTimer[STREAM::UNIQUE] = static_cast<u8>(mRegisterV[X] + (mRegisterV[X] == 1));
	}
	void CHIP8X::instruction_Fx1E(s32 X) noexcept {
		mRegisterI = (mRegisterI + mRegisterV[X]) & 0xFFF;
	}
	void CHIP8X::instruction_Fx29(s32 X) noexcept {
		mRegisterI = (mRegisterV[X] & 0xF) * 5;
	}
	void CHIP8X::instruction_Fx33(s32 X) noexcept {
		writeMemoryI(mRegisterV[X] / 100,     0);
		writeMemoryI(mRegisterV[X] / 10 % 10, 1);
		writeMemoryI(mRegisterV[X]      % 10, 2);
	}
	void CHIP8X::instruction_FN55(s32 N) noexcept {
		for (auto idx{ 0 }; idx <= N; ++idx)
			{ writeMemoryI(mRegisterV[idx], idx); }
		if (!Quirk.idxRegNoInc) [[likely]]
			{ mRegisterI = (mRegisterI + N + 1) & 0xFFF; }
	}
	void CHIP8X::instruction_FN65(s32 N) noexcept {
		for (auto idx{ 0 }; idx <= N; ++idx)
			{ mRegisterV[idx] = readMemoryI(idx); }
		if (!Quirk.idxRegNoInc) [[likely]]
			{ mRegisterI = (mRegisterI + N + 1) & 0xFFF; }
	}
	void CHIP8X::instruction_FxF8(s32 X) noexcept {
		setBuzzerPitch(mRegisterV[X]);
	}
	void CHIP8X::instruction_FxFB(s32) noexcept {
		triggerInterrupt(Interrupt::FRAME);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
