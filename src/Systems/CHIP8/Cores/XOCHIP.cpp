/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "XOCHIP.hpp"
#ifdef ENABLE_XOCHIP

#include "../../../Assistants/BasicVideoSpec.hpp"
#include "../../../Assistants/BasicAudioSpec.hpp"
#include "../../../Assistants/Well512.hpp"
#include "../../CoreRegistry.hpp"

REGISTER_CORE(XOCHIP, ".xo8")

/*==================================================================*/

XOCHIP::XOCHIP()
	: mDisplayBuffer{
		{cScreenSizeX, cScreenSizeY},
		{cScreenSizeX, cScreenSizeY},
		{cScreenSizeX, cScreenSizeY},
		{cScreenSizeX, cScreenSizeY},
	}
{
	Quirk.wrapSprite = true;

	std::fill(EXEC_POLICY(unseq)
		mMemoryBank.end() - cSafezoneOOB,
		mMemoryBank.end(), u8{ 0xFF }
	);

	copyGameToMemory(mMemoryBank.data() + cGameLoadPos);
	copyFontToMemory(mMemoryBank.data(), 0x50);
	copyColorsToCore(mBitColors.data());

	setDisplayResolution(cScreenSizeX, cScreenSizeY);
	setViewportSizes(true, cScreenSizeX, cScreenSizeY, cResSizeMult, 2);
	setSystemFramerate(cRefreshRate);

	mCurrentPC = cStartOffset;
	mTargetCPF = cInstSpeedLo;

	mAudio[STREAM::CHANN1].pause();
	mAudio[STREAM::CHANN2].pause();
}

/*==================================================================*/

void XOCHIP::instructionLoop() noexcept {

	auto cycleCount{ 0 };
	for (; cycleCount < mTargetCPF; ++cycleCount) {
		const auto HI{ mMemoryBank[mCurrentPC + 0u] };
		const auto LO{ mMemoryBank[mCurrentPC + 1u] };
		nextInstruction();

		switch (HI >> 4) {
			case 0x0:
				switch (HI << 8 | LO) {
					case 0x00C0: case 0x00C1: case 0x00C2: case 0x00C3:
					case 0x00C4: case 0x00C5: case 0x00C6: case 0x00C7:
					case 0x00C8: case 0x00C9: case 0x00CA: case 0x00CB:
					case 0x00CC: case 0x00CD: case 0x00CE: case 0x00CF:
						instruction_00CN(LO & 0xF);
						break;
					case 0x00D0: case 0x00D1: case 0x00D2: case 0x00D3:
					case 0x00D4: case 0x00D5: case 0x00D6: case 0x00D7:
					case 0x00D8: case 0x00D9: case 0x00DA: case 0x00DB:
					case 0x00DC: case 0x00DD: case 0x00DE: case 0x00DF:
						instruction_00DN(LO & 0xF);
						break;
					case 0x00E0:
						instruction_00E0();
						break;
					case 0x00EE:
						instruction_00EE();
						break;
					case 0x00FB:
						instruction_00FB();
						break;
					case 0x00FC:
						instruction_00FC();
						break;
					case 0x00FD:
						instruction_00FD();
						break;
					case 0x00FE:
						instruction_00FE();
						break;
					case 0x00FF:
						instruction_00FF();
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
					case 0x0:
						instruction_5xy0(HI & 0xF, LO >> 4);
						break;
					case 0x2:
						instruction_5xy2(HI & 0xF, LO >> 4);
						break;
					case 0x3:
						instruction_5xy3(HI & 0xF, LO >> 4);
						break;
					case 0x4:
						instruction_5xy4(HI & 0xF, LO >> 4);
						break;
					[[unlikely]]
					default:
						instructionError(HI, LO);
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
				switch (HI << 8 | LO) {
					case 0xF000:
						instruction_F000();
						break;
					case 0xF002:
						instruction_F002();
						break;
					default:
						switch (LO) {
							case 0x01:
								instruction_FN01(HI & 0xF);
								break;
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
							case 0x30:
								instruction_Fx30(HI & 0xF);
								break;
							case 0x33:
								instruction_Fx33(HI & 0xF);
								break;
							case 0x3A:
								instruction_Fx3A(HI & 0xF);
								break;
							case 0x55:
								instruction_FN55(HI & 0xF);
								break;
							case 0x65:
								instruction_FN65(HI & 0xF);
								break;
							case 0x75:
								instruction_FN75(HI & 0xF);
								break;
							case 0x85:
								instruction_FN85(HI & 0xF);
								break;
								[[unlikely]]
							default: instructionError(HI, LO);
						}
						break;
				}
				break;
		}
	}
	mElapsedCycles += cycleCount;
}

void XOCHIP::renderAudioData() {
	pushPatternTone(STREAM::UNIQUE);
	pushSquareTone(STREAM::BUZZER);

	setDisplayBorderColor(mBitColors[!!mAudioTimer[STREAM::BUZZER]]);
}

void XOCHIP::renderVideoData() {
	std::vector<u8> textureBuffer(mDisplayW * mDisplayH);

	std::for_each(EXEC_POLICY(unseq)
		textureBuffer.begin(),
		textureBuffer.end(),
		[&](auto& pixel) noexcept {
			const auto idx{ &pixel - textureBuffer.data() };
			::assign_cast(pixel,
				mDisplayBuffer[3](idx) << 3 |
				mDisplayBuffer[2](idx) << 2 |
				mDisplayBuffer[1](idx) << 1 |
				mDisplayBuffer[0](idx)
			);
		}
	);

	BVS->displayBuffer.write(textureBuffer,
		[pBitColors = mBitColors.data()](u32 pixel) noexcept {
			return static_cast<u32>(0xFF | pBitColors[pixel]);
		}
	);

	setViewportSizes(isResolutionChanged(false), mDisplayW, mDisplayH,
		isLargerDisplay() ? cResSizeMult / 2 : cResSizeMult, 2);
}

void XOCHIP::prepDisplayArea(const Resolution mode) {
	const bool wasLargerDisplay{ isLargerDisplay(mode != Resolution::LO) };
	isResolutionChanged(wasLargerDisplay != isLargerDisplay());

	const auto W{ isLargerDisplay() ? cScreenSizeX * 2 : cScreenSizeX };
	const auto H{ isLargerDisplay() ? cScreenSizeY * 2 : cScreenSizeY };

	setDisplayResolution(W, H);

	mDisplayBuffer[0].resizeClean(W, H);
	mDisplayBuffer[1].resizeClean(W, H);
	mDisplayBuffer[2].resizeClean(W, H);
	mDisplayBuffer[3].resizeClean(W, H);
};

void XOCHIP::setColorBit332(s32 bit, s32 color) noexcept {
	static constexpr u8 map3b[]{ 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xFF };
	static constexpr u8 map2b[]{ 0x00,             0x60,       0xA0,       0xFF };

	mBitColors[bit & 0xF] = {
		map3b[color >> 5 & 0x7], // red
		map3b[color >> 2 & 0x7], // green
		map2b[color      & 0x3], // blue
	};
}

void XOCHIP::pushPatternTone(u32 index) noexcept {
	const auto samplesTotal{ mAudio[index].getNextBufferSize(cRefreshRate) };
	auto samplesBuffer{ ::allocate<s16>(samplesTotal).as_value().release() };

	if (mAudioTimer[index]) {
		const auto audioTone{ std::pow(2.0f, (mAudioPitch - 64.0f) / 48.0f) };
		const auto audioStep{ 31.25f / mAudio[index].getFreq() * audioTone };

		for (auto& audioSample : std::span(samplesBuffer.get(), samplesTotal)) {
			const auto bitOffset{ static_cast<s32>(std::clamp(mAudioPhase[index] * 128.0f, 0.0f, 127.0f)) };
			const auto bytePhase{ 1 << (7 ^ (bitOffset & 7)) };
			audioSample = (mPatternBuf[bitOffset >> 3] & bytePhase ? 0x0F : 0xF0) << 8;

			mAudioPhase[index] += audioStep;
			mAudioPhase[index] -= static_cast<int>(mAudioPhase[index] );
		}
	} else { mAudioPhase[index] = 0.0f; }

	mAudio[index].pushAudioData(samplesBuffer.get(), samplesTotal);
}

/*==================================================================*/

void XOCHIP::skipInstruction() noexcept {
	mCurrentPC += NNNN() == 0xF000 ? 4 : 2;
}

void XOCHIP::scrollDisplayUP(s32 N) {
	if (mPlanarMask & 0x1) { mDisplayBuffer[0].shift(0, -N); }
	if (mPlanarMask & 0x2) { mDisplayBuffer[1].shift(0, -N); }
	if (mPlanarMask & 0x4) { mDisplayBuffer[2].shift(0, -N); }
	if (mPlanarMask & 0x8) { mDisplayBuffer[3].shift(0, -N); }
}
void XOCHIP::scrollDisplayDN(s32 N) {
	if (mPlanarMask & 0x1) { mDisplayBuffer[0].shift(0, +N); }
	if (mPlanarMask & 0x2) { mDisplayBuffer[1].shift(0, +N); }
	if (mPlanarMask & 0x4) { mDisplayBuffer[2].shift(0, +N); }
	if (mPlanarMask & 0x8) { mDisplayBuffer[3].shift(0, +N); }
}
void XOCHIP::scrollDisplayLT() {
	if (mPlanarMask & 0x1) { mDisplayBuffer[0].shift(-4, 0); }
	if (mPlanarMask & 0x2) { mDisplayBuffer[1].shift(-4, 0); }
	if (mPlanarMask & 0x4) { mDisplayBuffer[2].shift(-4, 0); }
	if (mPlanarMask & 0x8) { mDisplayBuffer[3].shift(-4, 0); }
}
void XOCHIP::scrollDisplayRT() {
	if (mPlanarMask & 0x1) { mDisplayBuffer[0].shift(+4, 0); }
	if (mPlanarMask & 0x2) { mDisplayBuffer[1].shift(+4, 0); }
	if (mPlanarMask & 0x4) { mDisplayBuffer[2].shift(+4, 0); }
	if (mPlanarMask & 0x8) { mDisplayBuffer[3].shift(+4, 0); }
}

/*==================================================================*/
	#pragma region 0 instruction branch

	void XOCHIP::instruction_00CN(s32 N) noexcept {
		if (Quirk.waitScroll) [[unlikely]]
			{ triggerInterrupt(Interrupt::FRAME); }
		if (N) { scrollDisplayDN(N); }
	}
	void XOCHIP::instruction_00DN(s32 N) noexcept {
		if (Quirk.waitScroll) [[unlikely]]
			{ triggerInterrupt(Interrupt::FRAME); }
		if (N) { scrollDisplayUP(N); }
	}
	void XOCHIP::instruction_00E0() noexcept {
		if (mPlanarMask & 0x1) { mDisplayBuffer[0].initialize(); }
		if (mPlanarMask & 0x2) { mDisplayBuffer[1].initialize(); }
		if (mPlanarMask & 0x4) { mDisplayBuffer[2].initialize(); }
		if (mPlanarMask & 0x8) { mDisplayBuffer[3].initialize(); }
	}
	void XOCHIP::instruction_00EE() noexcept {
		mCurrentPC = mStackBank[--mStackTop & 0xF];
	}
	void XOCHIP::instruction_00FB() noexcept {
		if (Quirk.waitScroll) [[unlikely]]
			{ triggerInterrupt(Interrupt::FRAME); }
		scrollDisplayRT();
	}
	void XOCHIP::instruction_00FC() noexcept {
		if (Quirk.waitScroll) [[unlikely]]
			{ triggerInterrupt(Interrupt::FRAME); }
		scrollDisplayLT();
	}
	void XOCHIP::instruction_00FD() noexcept {
		triggerInterrupt(Interrupt::SOUND);
	}
	void XOCHIP::instruction_00FE() noexcept {
		prepDisplayArea(Resolution::LO);
	}
	void XOCHIP::instruction_00FF() noexcept {
		prepDisplayArea(Resolution::HI);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 1 instruction branch

	void XOCHIP::instruction_1NNN(s32 NNN) noexcept {
		performProgJump(NNN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 2 instruction branch

	void XOCHIP::instruction_2NNN(s32 NNN) noexcept {
		mStackBank[mStackTop++ & 0xF] = mCurrentPC;
		performProgJump(NNN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 3 instruction branch

	void XOCHIP::instruction_3xNN(s32 X, s32 NN) noexcept {
		if (mRegisterV[X] == NN) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 4 instruction branch

	void XOCHIP::instruction_4xNN(s32 X, s32 NN) noexcept {
		if (mRegisterV[X] != NN) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 5 instruction branch

	void XOCHIP::instruction_5xy0(s32 X, s32 Y) noexcept {
		if (mRegisterV[X] == mRegisterV[Y]) { skipInstruction(); }
	}
	void XOCHIP::instruction_5xy2(s32 X, s32 Y) noexcept {
		const auto dist{ std::abs(X - Y) + 1 };
		const auto flip{ X < Y ? 1 : -1 };
		for (auto Z{ 0 }; Z < dist; ++Z)
			{ writeMemoryI(mRegisterV[X + Z * flip], Z); }
	}
	void XOCHIP::instruction_5xy3(s32 X, s32 Y) noexcept {
		const auto dist{ std::abs(X - Y) + 1 };
		const auto flip{ X < Y ? 1 : -1 };
		for (auto Z{ 0 }; Z < dist; ++Z)
			{ mRegisterV[X + Z * flip] = readMemoryI(Z); }
	}
	void XOCHIP::instruction_5xy4(s32 X, s32 Y) noexcept {
		const auto dist{ std::abs(X - Y) + 1 };
		const auto flip{ X < Y ? 1 : -1 };
		for (auto Z{ 0 }; Z < dist; ++Z)
			{ setColorBit332(X + Z * flip, readMemoryI(Z)); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 6 instruction branch

	void XOCHIP::instruction_6xNN(s32 X, s32 NN) noexcept {
		mRegisterV[X] = static_cast<u8>(NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 7 instruction branch

	void XOCHIP::instruction_7xNN(s32 X, s32 NN) noexcept {
		mRegisterV[X] += static_cast<u8>(NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 8 instruction branch

	void XOCHIP::instruction_8xy0(s32 X, s32 Y) noexcept {
		mRegisterV[X] = mRegisterV[Y];
	}
	void XOCHIP::instruction_8xy1(s32 X, s32 Y) noexcept {
		mRegisterV[X] |= mRegisterV[Y];
	}
	void XOCHIP::instruction_8xy2(s32 X, s32 Y) noexcept {
		mRegisterV[X] &= mRegisterV[Y];
	}
	void XOCHIP::instruction_8xy3(s32 X, s32 Y) noexcept {
		mRegisterV[X] ^= mRegisterV[Y];
	}
	void XOCHIP::instruction_8xy4(s32 X, s32 Y) noexcept {
		const auto sum{ mRegisterV[X] + mRegisterV[Y] };
		::assign_cast(mRegisterV[X], sum);
		::assign_cast(mRegisterV[0xF], sum >> 8);
	}
	void XOCHIP::instruction_8xy5(s32 X, s32 Y) noexcept {
		const bool nborrow{ mRegisterV[X] >= mRegisterV[Y] };
		::assign_cast(mRegisterV[X], mRegisterV[X] - mRegisterV[Y]);
		::assign_cast(mRegisterV[0xF], nborrow);
	}
	void XOCHIP::instruction_8xy7(s32 X, s32 Y) noexcept {
		const bool nborrow{ mRegisterV[Y] >= mRegisterV[X] };
		::assign_cast(mRegisterV[X], mRegisterV[Y] - mRegisterV[X]);
		::assign_cast(mRegisterV[0xF], nborrow);
	}
	void XOCHIP::instruction_8xy6(s32 X, s32 Y) noexcept {
		if (!Quirk.shiftVX) { mRegisterV[X] = mRegisterV[Y]; }
		const bool lsb{ (mRegisterV[X] & 1) == 1 };
		::assign_cast(mRegisterV[X], mRegisterV[X] >> 1);
		::assign_cast(mRegisterV[0xF], lsb);
	}
	void XOCHIP::instruction_8xyE(s32 X, s32 Y) noexcept {
		if (!Quirk.shiftVX) { mRegisterV[X] = mRegisterV[Y]; }
		const bool msb{ (mRegisterV[X] >> 7) == 1 };
		::assign_cast(mRegisterV[X], mRegisterV[X] << 1);
		::assign_cast(mRegisterV[0xF], msb);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 9 instruction branch

	void XOCHIP::instruction_9xy0(s32 X, s32 Y) noexcept {
		if (mRegisterV[X] != mRegisterV[Y]) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region A instruction branch

	void XOCHIP::instruction_ANNN(s32 NNN) noexcept {
		mRegisterI = NNN & 0xFFF;
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region B instruction branch

	void XOCHIP::instruction_BNNN(s32 NNN) noexcept {
		performProgJump(NNN + mRegisterV[0]);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region C instruction branch

	void XOCHIP::instruction_CxNN(s32 X, s32 NN) noexcept {
		::assign_cast(mRegisterV[X], RNG->next() & NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region D instruction branch

	void XOCHIP::drawByte(
		s32 X, s32 Y, s32 P,
		u32 DATA
	) noexcept {
		switch (DATA) {
			[[unlikely]]
			case 0b00000000:
				return;

			[[unlikely]]
			case 0b10000000:
				if (Quirk.wrapSprite) { X &= mDisplayWb; }
				if (X < mDisplayW) {
					if (!((mDisplayBuffer[P](X, Y) ^= 1) & 1))
						{ mRegisterV[0xF] = 1; }
				}
				return;

			[[likely]]
			default:
				if (Quirk.wrapSprite) { X &= mDisplayWb; }
				else if (X >= mDisplayW) { return; }

				for (auto B{ 0 }; B < 8; ++B, ++X &= mDisplayWb) {
					if (DATA & 0x80 >> B) {
						if (!((mDisplayBuffer[P](X, Y) ^= 1) & 1))
							{ mRegisterV[0xF] = 1; }
					}
					if (!Quirk.wrapSprite && X == mDisplayWb) { return; }
				}
				return;
		}
	}

	void XOCHIP::instruction_DxyN(s32 X, s32 Y, s32 N) noexcept {
		if (!mPlanarMask) {
			mRegisterV[0xF] = 0;
			return;
		}

		const auto pX{ mRegisterV[X] & mDisplayWb };
		const auto pY{ mRegisterV[Y] & mDisplayHb };

		mRegisterV[0xF] = 0;

		if (N == 0) {
			for (auto P{ 0 }, I{ 0 }; P < 4; ++P) {
				if (mPlanarMask & 1 << P) {
					for (auto tN{ 0 }, tY{ pY }; tN < 16; ++tN) {
						drawByte(pX + 0, tY, P, readMemoryI(I + tN * 2 + 0));
						drawByte(pX + 8, tY, P, readMemoryI(I + tN * 2 + 1));

						if (!Quirk.wrapSprite && tY == mDisplayHb) { break; }
						else { ++tY &= mDisplayHb; }
					}
					I += 32;
				}
			}
		} else [[likely]] {
			for (auto P{ 0 }, I{ 0 }; P < 4; ++P) {
				if (mPlanarMask & 1 << P) {
					for (auto tN{ 0 }, tY{ pY }; tN < N; ++tN) {
						drawByte(pX, tY, P, readMemoryI(I + tN));

						if (!Quirk.wrapSprite && tY == mDisplayHb) { break; }
						else { ++tY &= mDisplayHb; }
					}
					I += N;
				}
			}
		}
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region E instruction branch

	void XOCHIP::instruction_Ex9E(s32 X) noexcept {
		if (keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
	}
	void XOCHIP::instruction_ExA1(s32 X) noexcept {
		if (!keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region F instruction branch

	void XOCHIP::instruction_F000() noexcept {
		mRegisterI = NNNN();
		nextInstruction();
	}
	void XOCHIP::instruction_F002() noexcept {
		for (auto idx{ 0 }; idx < 16; ++idx)
			{ mPatternBuf[idx] = readMemoryI(idx); }
	}
	void XOCHIP::instruction_FN01(s32 N) noexcept {
		mPlanarMask = N;
	}
	void XOCHIP::instruction_Fx07(s32 X) noexcept {
		::assign_cast(mRegisterV[X], mDelayTimer);
	}
	void XOCHIP::instruction_Fx0A(s32 X) noexcept {
		triggerInterrupt(Interrupt::INPUT);
		mInputReg = &mRegisterV[X];
	}
	void XOCHIP::instruction_Fx15(s32 X) noexcept {
		mDelayTimer = mRegisterV[X];
	}
	void XOCHIP::instruction_Fx18(s32 X) noexcept {
		startAudioAtChannel(STREAM::UNIQUE, mRegisterV[X] + (mRegisterV[X] == 1));
	}
	void XOCHIP::instruction_Fx1E(s32 X) noexcept {
		mRegisterI = (mRegisterI + mRegisterV[X]) & 0xFFFF;
	}
	void XOCHIP::instruction_Fx29(s32 X) noexcept {
		mRegisterI = (mRegisterV[X] & 0xF) * 5;
	}
	void XOCHIP::instruction_Fx30(s32 X) noexcept {
		mRegisterI = (mRegisterV[X] & 0xF) * 10 + 80;
	}
	void XOCHIP::instruction_Fx33(s32 X) noexcept {
		writeMemoryI(mRegisterV[X] / 100,     0);
		writeMemoryI(mRegisterV[X] / 10 % 10, 1);
		writeMemoryI(mRegisterV[X]      % 10, 2);
	}
	void XOCHIP::instruction_Fx3A(s32 X) noexcept {
		mAudioPitch = mRegisterV[X];
	}
	void XOCHIP::instruction_FN55(s32 N) noexcept {
		SUGGEST_VECTORIZABLE_LOOP
		for (auto idx{ 0 }; idx <= N; ++idx) { writeMemoryI(mRegisterV[idx], idx); }
		mRegisterI = !Quirk.idxRegNoInc ? (mRegisterI + N + 1) & 0xFFFF : mRegisterI;
	}
	void XOCHIP::instruction_FN65(s32 N) noexcept {
		SUGGEST_VECTORIZABLE_LOOP
		for (auto idx{ 0 }; idx <= N; ++idx) { mRegisterV[idx] = readMemoryI(idx); }
		mRegisterI = !Quirk.idxRegNoInc ? (mRegisterI + N + 1) & 0xFFFF : mRegisterI;
	}
	void XOCHIP::instruction_FN75(s32 N) noexcept {
		setPermaRegs(N + 1);
	}
	void XOCHIP::instruction_FN85(s32 N) noexcept {
		getPermaRegs(N + 1);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

	
#endif
