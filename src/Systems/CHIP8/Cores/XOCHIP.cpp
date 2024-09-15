/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../../Assistants/BasicVideoSpec.hpp"
#include "../../../Assistants/BasicAudioSpec.hpp"
#include "../../../Assistants/Well512.hpp"

#include "XOCHIP.hpp"

/*==================================================================*/

XOCHIP::XOCHIP()
	: mDisplayBuffer{
		{cScreenSizeY, cScreenSizeX},
		{cScreenSizeY, cScreenSizeX},
		{cScreenSizeY, cScreenSizeX},
		{cScreenSizeY, cScreenSizeX},
	}
{
	if (getCoreState() != EmuState::FAILED) {
		Quirk.wrapSprite = true;

		copyGameToMemory(mMemoryBank.data(), cGameLoadPos);
		copyFontToMemory(mMemoryBank.data(), 0x0, 0x50);

		setDisplayResolution(cScreenSizeX, cScreenSizeY);

		BVS->setBackColor(cBitsColor[0]);
		BVS->createTexture(cScreenSizeX, cScreenSizeY);
		BVS->setAspectRatio(cScreenSizeX * 8, cScreenSizeY * 8, +2);

		std::copy_n(
			std::execution::unseq,
			cBitsColor.data(), 16,
			mBitsColor.data()
		);

		mCurrentPC = cStartOffset;
		mFramerate = cRefreshRate;
		mActiveCPF = cInstSpeedLo;
	}
}

/*==================================================================*/

void XOCHIP::handleTimerTick() noexcept {
	if (mDelayTimer) { --mDelayTimer; }
	if (mSoundTimer) { --mSoundTimer; }
}

void XOCHIP::instructionLoop() noexcept {

	auto cycleCount{ 0 };
	for (; cycleCount < mActiveCPF; ++cycleCount) {
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
	mTotalCycles += cycleCount;
}

void XOCHIP::renderAudioData() {
	std::vector<s8> samplesBuffer \
		(static_cast<usz>(ASB->getSampleRate(cRefreshRate)));

	static f32 wavePhase{};

	if (mSoundTimer) {
		if (isBuzzerEnabled()) {
			for (auto& sample : samplesBuffer) {
				sample    = static_cast<s8>(wavePhase > 0.5f ? 16 : -16);
				wavePhase = std::fmod(wavePhase + mBuzzerTone, 1.0f);
			}
			BVS->setFrameColor(cBitsColor[0], cBitsColor[1]);
		} else {
			const auto audioStep{ std::pow(2.0f, (mAudioPitch - 64.0f) / 48.0f) };
			const auto audioTone{ 31.25f / ASB->getFrequency() * audioStep };

			for (auto& sample : samplesBuffer) {
				const auto step{ static_cast<s32>(std::clamp(wavePhase * 128.0f, 0.0f, 127.0f)) };
				const auto mask{ 1 << (7 ^ step & 7) };
				sample    = mPatternBuf[step >> 3] & mask ? 16 : -16;
				wavePhase = std::fmod(wavePhase + audioTone, 1.0f);
			}
			BVS->setFrameColor(cBitsColor[0], cBitsColor[0]);
		}
	} else {
		wavePhase = 0.0f;
		isBuzzerEnabled(false);
		BVS->setFrameColor(cBitsColor[0], cBitsColor[0]);
	}

	ASB->pushAudioData<s8>(samplesBuffer);
}

void XOCHIP::renderVideoData() {
	std::vector<u8> textureBuffer(mDisplayW * mDisplayH);

	std::for_each(
		std::execution::unseq,
		textureBuffer.begin(),
		textureBuffer.end(),
		[&](u8& pixel) noexcept {
			const auto idx{ &pixel - textureBuffer.data() };
			pixel = mDisplayBuffer[3].at_raw(idx) << 3
				  | mDisplayBuffer[2].at_raw(idx) << 2
				  | mDisplayBuffer[1].at_raw(idx) << 1
				  | mDisplayBuffer[0].at_raw(idx);
		}
	);

	BVS->modifyTexture<u8>(textureBuffer,
		[this](const u32 pixel) noexcept {
			return 0xFF000000 | mBitsColor[pixel];
		}
	);
}

void XOCHIP::prepDisplayArea(const Resolution mode) {
	isLoresExtended(mode != Resolution::LO);

	const auto W{ isLoresExtended() ? 128 : 64 };
	const auto H{ isLoresExtended() ?  64 : 32 };

	BVS->createTexture(W, H);
	setDisplayResolution(W, H);
	
	mDisplayBuffer[0].resize(false, H, W);
	mDisplayBuffer[1].resize(false, H, W);
	mDisplayBuffer[2].resize(false, H, W);
	mDisplayBuffer[3].resize(false, H, W);
};

void XOCHIP::setColorBit332(const s32 bit, const s32 color) noexcept {
	static constexpr u8 map3b[]{ 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xFF };
	static constexpr u8 map2b[]{ 0x00,             0x60,       0xA0,       0xFF };

	mBitsColor[bit & 0xF] = map3b[color >> 5 & 0x7] << 16 // red
						  | map3b[color >> 2 & 0x7] <<  8 // green
						  | map2b[color      & 0x3];      // blue
}

/*==================================================================*/

void XOCHIP::nextInstruction() noexcept {
	mCurrentPC += 2;
}

void XOCHIP::skipInstruction() noexcept {
	mCurrentPC += NNNN() == 0xF000 ? 4 : 2;
}

void XOCHIP::jumpProgramTo(const u32 next) noexcept {
	const auto NNN{ next & 0xFFF };
	if (mCurrentPC - 2u != NNN) [[likely]] {
		mCurrentPC = NNN & 0xFFF;
	} else {
		triggerInterrupt(Interrupt::SOUND);
	}
}

void XOCHIP::scrollDisplayUP(const s32 N) {
	if (!mPlanarMask) { return; }

	for (auto P{ 0 }; P < 4; ++P) {
		if (mPlanarMask & (1 << P))
			{ mDisplayBuffer[P].shift(-N, 0); }
	}
}
void XOCHIP::scrollDisplayDN(const s32 N) {
	if (!mPlanarMask) { return; }

	for (auto P{ 0 }; P < 4; ++P) {
		if (mPlanarMask & (1 << P))
			{ mDisplayBuffer[P].shift(+N, 0); }
	}
}
void XOCHIP::scrollDisplayLT() {
	if (!mPlanarMask) { return; }

	for (auto P{ 0 }; P < 4; ++P) {
		if (mPlanarMask & (1 << P)) 
			{ mDisplayBuffer[P].shift(0, -4); }
	}
}
void XOCHIP::scrollDisplayRT() {
	if (!mPlanarMask) { return; }

	for (auto P{ 0 }; P < 4; ++P) {
		if (mPlanarMask & (1 << P))
			{ mDisplayBuffer[P].shift(0, +4); }
	}
}

/*==================================================================*/
	#pragma region 0 instruction branch

	void XOCHIP::instruction_00CN(const s32 N) noexcept {
		if (Quirk.waitScroll) [[unlikely]]
			{ triggerInterrupt(Interrupt::FRAME); }
		if (N) { scrollDisplayDN(N); }
	}
	void XOCHIP::instruction_00DN(const s32 N) noexcept {
		if (Quirk.waitScroll) [[unlikely]]
			{ triggerInterrupt(Interrupt::FRAME); }
		if (N) { scrollDisplayUP(N); }
	}
	void XOCHIP::instruction_00E0() noexcept {
		for (auto P{ 0 }; P < 4; ++P) {
			if (!(mPlanarMask & (1 << P))) { continue; }
			mDisplayBuffer[P].wipeAll();
		}
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

	void XOCHIP::instruction_1NNN(const s32 NNN) noexcept {
		jumpProgramTo(NNN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 2 instruction branch

	void XOCHIP::instruction_2NNN(const s32 NNN) noexcept {
		mStackBank[mStackTop++ & 0xF] = mCurrentPC;
		jumpProgramTo(NNN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 3 instruction branch

	void XOCHIP::instruction_3xNN(const s32 X, const s32 NN) noexcept {
		if (mRegisterV[X] == NN) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 4 instruction branch

	void XOCHIP::instruction_4xNN(const s32 X, const s32 NN) noexcept {
		if (mRegisterV[X] != NN) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 5 instruction branch

	void XOCHIP::instruction_5xy0(const s32 X, const s32 Y) noexcept {
		if (mRegisterV[X] == mRegisterV[Y]) { skipInstruction(); }
	}
	void XOCHIP::instruction_5xy2(const s32 X, const s32 Y) noexcept {
		const auto dist{ std::abs(X - Y) + 1 };
		if (X < Y) {
			for (auto Z{ 0 }; Z < dist; ++Z) {
				writeMemoryI(mRegisterV[X + Z], Z);
			}
		} else {
			for (auto Z{ 0 }; Z < dist; ++Z) {
				writeMemoryI(mRegisterV[X - Z], Z);
			}
		}
	}
	void XOCHIP::instruction_5xy3(const s32 X, const s32 Y) noexcept {
		const auto dist{ std::abs(X - Y) + 1 };
		if (X < Y) {
			for (auto Z{ 0 }; Z < dist; ++Z) {
				mRegisterV[X + Z] = readMemoryI(Z);
			}
		} else {
			for (auto Z{ 0 }; Z < dist; ++Z) {
				mRegisterV[X - Z] = readMemoryI(Z);
			}
		}
	}
	void XOCHIP::instruction_5xy4(const s32 X, const s32 Y) noexcept {
		const auto dist{ std::abs(X - Y) + 1 };
		if (X < Y) {
			for (auto Z{ 0 }; Z < dist; ++Z) {
				setColorBit332(X + Z, readMemoryI(Z));
			}
		} else {
			for (auto Z{ 0 }; Z < dist; ++Z) {
				setColorBit332(X - Z, readMemoryI(Z));
			}
		}
		BVS->setBackColor(mBitsColor[0]);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 6 instruction branch

	void XOCHIP::instruction_6xNN(const s32 X, const s32 NN) noexcept {
		mRegisterV[X] = static_cast<u8>(NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 7 instruction branch

	void XOCHIP::instruction_7xNN(const s32 X, const s32 NN) noexcept {
		mRegisterV[X] += static_cast<u8>(NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 8 instruction branch

	void XOCHIP::instruction_8xy0(const s32 X, const s32 Y) noexcept {
		mRegisterV[X] = mRegisterV[Y];
	}
	void XOCHIP::instruction_8xy1(const s32 X, const s32 Y) noexcept {
		mRegisterV[X] |= mRegisterV[Y];
	}
	void XOCHIP::instruction_8xy2(const s32 X, const s32 Y) noexcept {
		mRegisterV[X] &= mRegisterV[Y];
	}
	void XOCHIP::instruction_8xy3(const s32 X, const s32 Y) noexcept {
		mRegisterV[X] ^= mRegisterV[Y];
	}
	void XOCHIP::instruction_8xy4(const s32 X, const s32 Y) noexcept {
		const auto sum{ mRegisterV[X] + mRegisterV[Y] };
		mRegisterV[X]   = static_cast<u8>(sum);
		mRegisterV[0xF] = static_cast<u8>(sum >> 8);
	}
	void XOCHIP::instruction_8xy5(const s32 X, const s32 Y) noexcept {
		const bool nborrow{ mRegisterV[X] >= mRegisterV[Y] };
		mRegisterV[X]   = mRegisterV[X] - mRegisterV[Y];
		mRegisterV[0xF] = nborrow;
	}
	void XOCHIP::instruction_8xy7(const s32 X, const s32 Y) noexcept {
		const bool nborrow{ mRegisterV[Y] >= mRegisterV[X] };
		mRegisterV[X]   = mRegisterV[Y] - mRegisterV[X];
		mRegisterV[0xF] = nborrow;
	}
	void XOCHIP::instruction_8xy6(const s32 X, const s32 Y) noexcept {
		if (!Quirk.shiftVX) { mRegisterV[X] = mRegisterV[Y]; }
		const bool lsb{ (mRegisterV[X] & 1) == 1 };
		mRegisterV[X]   = mRegisterV[X] >> 1;
		mRegisterV[0xF] = lsb;
	}
	void XOCHIP::instruction_8xyE(const s32 X, const s32 Y) noexcept {
		if (!Quirk.shiftVX) { mRegisterV[X] = mRegisterV[Y]; }
		const bool msb{ (mRegisterV[X] >> 7) == 1 };
		mRegisterV[X]   = mRegisterV[X] << 1;
		mRegisterV[0xF] = msb;
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 9 instruction branch

	void XOCHIP::instruction_9xy0(const s32 X, const s32 Y) noexcept {
		if (mRegisterV[X] != mRegisterV[Y]) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region A instruction branch

	void XOCHIP::instruction_ANNN(const s32 NNN) noexcept {
		mRegisterI = NNN & 0xFFF;
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region B instruction branch

	void XOCHIP::instruction_BNNN(const s32 NNN) noexcept {
		jumpProgramTo(NNN + mRegisterV[0]);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region C instruction branch

	void XOCHIP::instruction_CxNN(const s32 X, const s32 NN) noexcept {
		mRegisterV[X] = static_cast<u8>(Wrand->get() & NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region D instruction branch

	void XOCHIP::drawByte(
		s32 X, s32 Y, const s32 P,
		const u32 DATA
	) noexcept {
		switch (DATA) {
			[[unlikely]]
			case 0b00000000:
				return;

			[[unlikely]]
			case 0b10000000:
				if (Quirk.wrapSprite) { X &= mDisplayWb; }
				if (X < mDisplayW) {
					if (!((mDisplayBuffer[P].at_raw(Y, X) ^= 1) & 1))
						{ mRegisterV[0xF] = 1; }
				}
				return;

			[[likely]]
			default:
				if (Quirk.wrapSprite) { X &= mDisplayWb; }
				else if (X >= mDisplayW) { return; }

				for (auto B{ 0 }; B < 8; ++B, ++X &= mDisplayWb) {
					if (DATA & 0x80 >> B) {
						if (!((mDisplayBuffer[P].at_raw(Y, X) ^= 1) & 1))
							{ mRegisterV[0xF] = 1; }
					}
					if (!Quirk.wrapSprite && X == mDisplayWb) { return; }
				}
				return;
		}
	}

	void XOCHIP::instruction_DxyN(const s32 X, const s32 Y, const s32 N) noexcept {
		if (!mPlanarMask) {
			mRegisterV[0xF] = 0;
		} else {
			const auto pX{ mRegisterV[X] & mDisplayWb };
			const auto pY{ mRegisterV[Y] & mDisplayHb };

			mRegisterV[0xF] = 0;

			if (N == 0) {
				for (auto P{ 0 }, I{ 0 }; P < 4; ++P, I += 32) {
					if (!(mPlanarMask & 1 << P)) { continue; }

					for (auto tN{ 0 }, tY{ pY }; tN < 32;) {
						drawByte(pX + 0, tY, P, readMemoryI(I + tN + 0));
						drawByte(pX + 8, tY, P, readMemoryI(I + tN + 1));
						if (!Quirk.wrapSprite && tY == mDisplayHb) { break; }
						else { tN += 2; ++tY &= mDisplayHb; }
					}
				}
			} else [[likely]] {
				for (auto P{ 0 }, I{ 0 }; P < 4; ++P, I += N) {
					if (!(mPlanarMask & 1 << P)) { continue; }

					for (auto tN{ 0 }, tY{ pY }; tN < N;) {
						drawByte(pX, tY, P, readMemoryI(I + tN));
						if (!Quirk.wrapSprite && tY == mDisplayHb) { break; }
						else { tN += 1; ++tY &= mDisplayHb; }
					}
				}
			}
		}
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region E instruction branch

	void XOCHIP::instruction_Ex9E(const s32 X) noexcept {
		if (keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
	}
	void XOCHIP::instruction_ExA1(const s32 X) noexcept {
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
		for (auto idx{ 0 }; idx < 16; ++idx) {
			mPatternBuf[idx] = readMemoryI(idx);
		}
		isBuzzerEnabled(false);
	}
	void XOCHIP::instruction_FN01(const s32 N) noexcept {
		mPlanarMask = N;
	}
	void XOCHIP::instruction_Fx07(const s32 X) noexcept {
		mRegisterV[X] = static_cast<u8>(mDelayTimer);
	}
	void XOCHIP::instruction_Fx0A(const s32 X) noexcept {
		triggerInterrupt(Interrupt::INPUT);
		mInputReg = &mRegisterV[X];
	}
	void XOCHIP::instruction_Fx15(const s32 X) noexcept {
		mDelayTimer = static_cast<u8>(mRegisterV[X]);
	}
	void XOCHIP::instruction_Fx18(const s32 X) noexcept {
		mBuzzerTone = calcBuzzerTone();
		mSoundTimer = mRegisterV[X] + (mRegisterV[X] == 1);
	}
	void XOCHIP::instruction_Fx1E(const s32 X) noexcept {
		mRegisterI = mRegisterI + mRegisterV[X] & 0xFFFF;
	}
	void XOCHIP::instruction_Fx29(const s32 X) noexcept {
		mRegisterI = (mRegisterV[X] & 0xF) * 5;
	}
	void XOCHIP::instruction_Fx30(const s32 X) noexcept {
		mRegisterI = (mRegisterV[X] & 0xF) * 10 + 80;
	}
	void XOCHIP::instruction_Fx33(const s32 X) noexcept {
		writeMemoryI(mRegisterV[X] / 100,     0);
		writeMemoryI(mRegisterV[X] / 10 % 10, 1);
		writeMemoryI(mRegisterV[X]      % 10, 2);
	}
	void XOCHIP::instruction_Fx3A(const s32 X) noexcept {
		mAudioPitch = mRegisterV[X];
		isBuzzerEnabled(false);
	}
	void XOCHIP::instruction_FN55(const s32 N) noexcept {
		for (auto idx{ 0 }; idx <= N; ++idx)
			{ writeMemoryI(mRegisterV[idx], idx); }
		if (!Quirk.idxRegNoInc) [[likely]]
			{ mRegisterI = mRegisterI + N + 1 & 0xFFFF; }
	}
	void XOCHIP::instruction_FN65(const s32 N) noexcept {
		for (auto idx{ 0 }; idx <= N; ++idx)
			{ mRegisterV[idx] = readMemoryI(idx); }
		if (!Quirk.idxRegNoInc) [[likely]]
			{ mRegisterI = mRegisterI + N + 1 & 0xFFFF; }
	}
	void XOCHIP::instruction_FN75(const s32 N) noexcept {
		if (setPermaRegs(N + 1)) [[unlikely]]
			{ triggerCritError("Error :: Failed writing persistent registers!"); }
	}
	void XOCHIP::instruction_FN85(const s32 N) noexcept {
		if (getPermaRegs(N + 1)) [[unlikely]]
			{ triggerCritError("Error :: Failed reading persistent registers!"); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
