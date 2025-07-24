/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "MEGACHIP.hpp"
#if defined(ENABLE_CHIP8_SYSTEM) && defined(ENABLE_MEGACHIP)

#include "../../../Assistants/BasicVideoSpec.hpp"
#include "../../../Assistants/GlobalAudioBase.hpp"
#include "../../../Assistants/Well512.hpp"
#include "../../CoreRegistry.hpp"

REGISTER_CORE(MEGACHIP, ".mc8")

/*==================================================================*/

MEGACHIP::MEGACHIP() {
	copyGameToMemory(mMemoryBank.data() + cGameLoadPos);
	copyFontToMemory(mMemoryBank.data(), 0xB4);

	setViewportSizes(true, cScreenSizeX, cScreenSizeY, cResSizeMult, 2);
	setSystemFramerate(cRefreshRate);

	mVoices[VOICE::UNIQUE].userdata = &mAudioTimers[VOICE::UNIQUE];
	mVoices[VOICE::BUZZER].userdata = &mAudioTimers[VOICE::BUZZER];

	mCurrentPC = cStartOffset;
	
	prepDisplayArea(Resolution::LO);
	setNewBlendAlgorithm(BlendMode::ALPHA_BLEND);
	initializeFontColors();
}

/*==================================================================*/

void MEGACHIP::instructionLoop() noexcept {

	auto cycleCount{ 0 };
	for (; cycleCount < mTargetCPF; ++cycleCount) {
		const auto HI{ mMemoryBank[mCurrentPC + 0u] };
		const auto LO{ mMemoryBank[mCurrentPC + 1u] };
		nextInstruction();

		switch (HI >> 4) {
			case 0x0:
				if (isManualRefresh()) {
					switch (HI << 8 | LO) {
						case 0x0010:
							instruction_0010();
							break;
						case 0x0700:
							instruction_0700();
							break;
						case 0x0600: case 0x0601: case 0x0602: case 0x0603:
						case 0x0604: case 0x0605: case 0x0606: case 0x0607:
						case 0x0608: case 0x0609: case 0x060A: case 0x060B:
						case 0x060C: case 0x060D: case 0x060E: case 0x060F:
							instruction_060N(LO & 0xF);
							break;
						case 0x0800: case 0x0801: case 0x0802: case 0x0803:
						case 0x0804: case 0x0805: case 0x0806: case 0x0807:
						case 0x0808: case 0x0809: case 0x080A: case 0x080B:
						case 0x080C: case 0x080D: case 0x080E: case 0x080F:
							instruction_080N(LO & 0xF);
							break;
						/* padded */ case 0x00B1: case 0x00B2: case 0x00B3:
						case 0x00B4: case 0x00B5: case 0x00B6: case 0x00B7:
						case 0x00B8: case 0x00B9: case 0x00BA: case 0x00BB:
						case 0x00BC: case 0x00BD: case 0x00BE: case 0x00BF:
							instruction_00BN(LO & 0xF);
							break;
						/* padded */ case 0x00C1: case 0x00C2: case 0x00C3:
						case 0x00C4: case 0x00C5: case 0x00C6: case 0x00C7:
						case 0x00C8: case 0x00C9: case 0x00CA: case 0x00CB:
						case 0x00CC: case 0x00CD: case 0x00CE: case 0x00CF:
							instruction_00CN(LO & 0xF);
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
						default:
							switch (HI & 0xF) {
								case 0x01:
									instruction_01NN(LO);
									break;
								case 0x02:
									instruction_02NN(LO);
									break;
								case 0x03:
									instruction_03NN(LO);
									break;
								case 0x04:
									instruction_04NN(LO);
									break;
								case 0x05:
									instruction_05NN(LO);
									break;
								case 0x09:
									instruction_09NN(LO);
									break;
								[[unlikely]]
								default: instructionError(HI, LO);
							}
					}
				} else {
					switch (HI << 8 | LO) {
						case 0x0011:
							instruction_0011();
							break;
						/* padded */ case 0x00B1: case 0x00B2: case 0x00B3:
						case 0x00B4: case 0x00B5: case 0x00B6: case 0x00B7:
						case 0x00B8: case 0x00B9: case 0x00BA: case 0x00BB:
						case 0x00BC: case 0x00BD: case 0x00BE: case 0x00BF:
							instruction_00BN(LO & 0xF);
							break;
						/* padded */ case 0x00C1: case 0x00C2: case 0x00C3:
						case 0x00C4: case 0x00C5: case 0x00C6: case 0x00C7:
						case 0x00C8: case 0x00C9: case 0x00CA: case 0x00CB:
						case 0x00CC: case 0x00CD: case 0x00CE: case 0x00CF:
							instruction_00CN(LO & 0xF);
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
				instruction_BXNN(HI & 0xF, HI << 8 | LO);
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
					case 0x30:
						instruction_Fx30(HI & 0xF);
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
	}
	mElapsedCycles = cycleCount;
}

void MEGACHIP::renderAudioData() {
	if (isManualRefresh()) {
		mixAudioData({
			{ makeByteWave,  &mVoices[VOICE::UNIQUE] },
			{ makePulseWave, &mVoices[VOICE::BUZZER] },
		});

		setDisplayBorderColor(sBitColors[!!mAudioTimers[VOICE::BUZZER]]);
	}
	else {
		mixAudioData({
			{ makePulseWave, &mVoices[VOICE::ID_0] },
			{ makePulseWave, &mVoices[VOICE::ID_1] },
			{ makePulseWave, &mVoices[VOICE::ID_2] },
			{ makePulseWave, &mVoices[VOICE::BUZZER] },
		});

		setDisplayBorderColor(sBitColors[!!::accumulate(mAudioTimers)]);
	}
}

void MEGACHIP::renderVideoData() {
	if (!isManualRefresh()) {
		BVS->displayBuffer.write(mDisplayBuffer, isUsingPixelTrails()
			? [](u32 pixel) noexcept {
				return cPixelOpacity[pixel] | sBitColors[pixel != 0];
			}
			: [](u32 pixel) noexcept {
				return 0xFFu | sBitColors[pixel >> 3];
			}
		);

		setViewportSizes(isResolutionChanged(false), cScreenSizeX, cScreenSizeY, cResSizeMult, 2);
	} else {
		setViewportSizes(isResolutionChanged(false), cScreenMegaX, cScreenMegaY, cResSizeMult / 2, 2);
	}
}

void MEGACHIP::prepDisplayArea(Resolution mode) {
	const bool wasManualRefresh{ isManualRefresh(mode == Resolution::MC) };
	isResolutionChanged(wasManualRefresh != isManualRefresh());

	if (isManualRefresh()) {
		mDisplay.set(cScreenMegaX, cScreenMegaY);

		Quirk.waitVblank = false;
		mTargetCPF = cInstSpeedMC;
	}
	else {
		isLargerDisplay(mode != Resolution::LO);
		mDisplay.set(cScreenSizeX, cScreenSizeY);

		Quirk.waitVblank = !isLargerDisplay();
		mTargetCPF = isLargerDisplay() ? cInstSpeedLo : cInstSpeedHi;
	}
};

/*==================================================================*/

void MEGACHIP::skipInstruction() noexcept {
	mCurrentPC += readMemory(mCurrentPC) == 0x01 ? 4 : 2;
}

void MEGACHIP::scrollDisplayUP(s32 N) {
	mDisplayBuffer.shift(0, -N);
}
void MEGACHIP::scrollDisplayDN(s32 N) {
	mDisplayBuffer.shift(0, +N);
}
void MEGACHIP::scrollDisplayLT() {
	mDisplayBuffer.shift(-4, 0);
}
void MEGACHIP::scrollDisplayRT() {
	mDisplayBuffer.shift(+4, 0);
}

/*==================================================================*/

void MEGACHIP::initializeFontColors() noexcept {
	for (auto i{ 0 }; i < 10; ++i) {
		//const auto mult{ 1.0f - 0.045f * i };
		//const auto R{ 0xFF * mult * 1.03f };
		//const auto G{ 0xFF * mult * 1.14f };
		//const auto B{ 0xFF * mult * 1.21f };
		//
		//mFontColor[i] = {
		//	static_cast<u8>(std::min(std::round(R), 255.0f)),
		//	static_cast<u8>(std::min(std::round(G), 255.0f)),
		//	static_cast<u8>(std::min(std::round(B), 255.0f)),
		//};

		const auto mult{ 255 - 11 * i };
		
		mFontColor[i] = RGBA{
			EzMaths::fixedMul8(0xFF, u8(std::min(mult * 264, 255))),
			EzMaths::fixedMul8(0xFF, u8(std::min(mult * 291, 255))),
			EzMaths::fixedMul8(0xFF, u8(std::min(mult * 309, 255))),
		};
	}
}

RGBA MEGACHIP::blendPixel(RGBA src, RGBA dst, BlendFunction func, u8 opacity) noexcept {
	if (auto alpha{ EzMaths::fixedMul8(src.A, opacity) }) [[likely]] {
		RGBA blend{
			func(src.R, dst.R),
			func(src.G, dst.G),
			func(src.B, dst.B)
		};

		if (alpha != 0xFFu) [[likely]] {
			return RGBA::lerp(dst, blend, alpha);
		} else {
			return blend;
		}
	}
	return dst;
}

void MEGACHIP::setNewBlendAlgorithm(s32 mode) noexcept {
	switch (mode) {
		case BlendMode::LINEAR_DODGE:
			mBlendFunc = [](u8 src, u8 dst)
				noexcept { return u8(std::min(src + dst, 0xFF)); };
			break;

		case BlendMode::MULTIPLY:
			mBlendFunc = [](u8 src, u8 dst)
				noexcept { return EzMaths::fixedMul8(src, dst); };
			break;

		default:
		case BlendMode::ALPHA_BLEND:
			mBlendFunc = [](u8 src, u8)
				noexcept { return src; };
			break;
	}
}

void MEGACHIP::scrapAllVideoBuffers() {
	mLastRenderBuffer.initialize();
	mBackgroundBuffer.initialize();
	mCollisionMap.initialize();
}

void MEGACHIP::flushAllVideoBuffers() {
	BVS->displayBuffer.write(mBackgroundBuffer);

	mLastRenderBuffer = mBackgroundBuffer;
	mBackgroundBuffer.initialize();
	mCollisionMap.initialize();
}

void MEGACHIP::blendAndFlushBuffers() const {
	BVS->displayBuffer.write(
		mLastRenderBuffer,
		mBackgroundBuffer,
		[this](RGBA src, RGBA dst) noexcept {
			return blendPixel(src, dst, \
				mBlendFunc, u8(mTexture.opacity));
		}
	);
}

void MEGACHIP::startAudioTrack(bool repeat) noexcept {
	if (auto* stream{ mAudioDevice.at(STREAM::MAIN) }) {

		mTrack.loop = repeat;
		mTrack.data = &mMemoryBank[mRegisterI + 6];
		mTrack.size = readMemoryI(2) << 16
					| readMemoryI(3) <<  8
					| readMemoryI(4);

		const bool oob{ mTrack.data + mTrack.size > &mMemoryBank.back() };
		if (!mTrack.size || oob) { mTrack.reset(); } else {
			mVoices[VOICE::UNIQUE].setPhase(0.0).setStep(
				(readMemoryI(0) << 8 | readMemoryI(1)) / f64(mTrack.size) / stream->getFreq()
			).userdata = &mTrack;
		}
	}
}

void MEGACHIP::makeByteWave(f32* data, u32 size, Voice* voice, Stream*) noexcept {
	if (!voice || !voice->userdata) [[unlikely]] { return; }
	if (auto* track{ static_cast<TrackData*>(voice->userdata) }) {
		if (!track->isOn()) { return; }

		for (auto i{ 0u }; i < size; ++i) {
			const auto head{ voice->peekRawPhase(i) };
			if (!track->loop && head >= 1.0f) {
				track->reset(); return;
			} else {
				::assign_cast_add(data[i], (1.0f / 128) * \
					track->pos(head));
			}
		}
		voice->stepPhase(size);
	}
}

void MEGACHIP::scrollBuffersUP(s32 N) {
	mLastRenderBuffer.shift(0, -N);
	blendAndFlushBuffers();
}
void MEGACHIP::scrollBuffersDN(s32 N) {
	mLastRenderBuffer.shift(0, +N);
	blendAndFlushBuffers();
}
void MEGACHIP::scrollBuffersLT() {
	mLastRenderBuffer.shift(-4, 0);
	blendAndFlushBuffers();
}
void MEGACHIP::scrollBuffersRT() {
	mLastRenderBuffer.shift(+4, 0);
	blendAndFlushBuffers();
}

/*==================================================================*/
	#pragma region 0 instruction branch

	void MEGACHIP::instruction_00BN(s32 N) noexcept {
		if (isManualRefresh()) {
			scrollBuffersUP(N);
		} else {
			scrollDisplayUP(N);
		}
	}
	void MEGACHIP::instruction_00CN(s32 N) noexcept {
		if (isManualRefresh()) {
			scrollBuffersDN(N);
		} else {
			scrollDisplayDN(N);
		}
	}
	void MEGACHIP::instruction_00E0() noexcept {
		triggerInterrupt(Interrupt::FRAME);
		if (isManualRefresh()) {
			flushAllVideoBuffers();
		} else {
			mDisplayBuffer.initialize();
		}
	}
	void MEGACHIP::instruction_00EE() noexcept {
		mCurrentPC = mStackBank[--mStackTop & 0xF];
	}
	void MEGACHIP::instruction_00FB() noexcept {
		if (isManualRefresh()) {
			scrollBuffersRT();
		} else {
			scrollDisplayRT();
		}
	}
	void MEGACHIP::instruction_00FC() noexcept {
		if (isManualRefresh()) {
			scrollBuffersLT();
		} else {
			scrollDisplayLT();
		}
	}
	void MEGACHIP::instruction_00FD() noexcept {
		triggerInterrupt(Interrupt::SOUND);
	}
	void MEGACHIP::instruction_00FE() noexcept {
		triggerInterrupt(Interrupt::FRAME);
		prepDisplayArea(Resolution::LO);
	}
	void MEGACHIP::instruction_00FF() noexcept {
		triggerInterrupt(Interrupt::FRAME);
		prepDisplayArea(Resolution::HI);
	}

	void MEGACHIP::instruction_0010() noexcept {
		triggerInterrupt(Interrupt::FRAME);

		mTrack.reset();
		flushAllVideoBuffers();

		prepDisplayArea(Resolution::LO);
	}
	void MEGACHIP::instruction_0011() noexcept {
		triggerInterrupt(Interrupt::FRAME);

		mTrack.reset();
		scrapAllVideoBuffers();

		prepDisplayArea(Resolution::MC);
	}
	void MEGACHIP::instruction_01NN(s32 NN) noexcept {
		mRegisterI = (NN << 16) | NNNN();
		nextInstruction();
	}
	void MEGACHIP::instruction_02NN(s32 NN) noexcept {
		for (auto pos{ 0 }, offset{ 0 }; pos < NN; offset += 4) {
			mColorPalette(++pos) = {
				readMemoryI(offset + 1),
				readMemoryI(offset + 2),
				readMemoryI(offset + 3),
				readMemoryI(offset + 0),
			};
		}
	}
	void MEGACHIP::instruction_03NN(s32 NN) noexcept {
		mTexture.W = NN ? NN : 256;
	}
	void MEGACHIP::instruction_04NN(s32 NN) noexcept {
		mTexture.H = NN ? NN : 256;
	}
	void MEGACHIP::instruction_05NN(s32 NN) noexcept {
		BVS->setViewportAlpha(NN);
	}
	void MEGACHIP::instruction_060N(s32 N) noexcept {
		startAudioTrack(N == 0);
	}
	void MEGACHIP::instruction_0700() noexcept {
		mTrack.reset();
	}
	void MEGACHIP::instruction_080N(s32 N) noexcept {
		static constexpr u8 opacity[]{ 0xFF, 0x3F, 0x7F, 0xBF };
		::assign_cast(mTexture.opacity, opacity[N > 3 ? 0 : N]);
		setNewBlendAlgorithm(N);
	}
	void MEGACHIP::instruction_09NN(s32 NN) noexcept {
		mTexture.collide = NN;
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 1 instruction branch

	void MEGACHIP::instruction_1NNN(s32 NNN) noexcept {
		performProgJump(NNN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 2 instruction branch

	void MEGACHIP::instruction_2NNN(s32 NNN) noexcept {
		mStackBank[mStackTop++ & 0xF] = mCurrentPC;
		performProgJump(NNN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 3 instruction branch

	void MEGACHIP::instruction_3xNN(s32 X, s32 NN) noexcept {
		if (mRegisterV[X] == NN) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 4 instruction branch

	void MEGACHIP::instruction_4xNN(s32 X, s32 NN) noexcept {
		if (mRegisterV[X] != NN) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 5 instruction branch

	void MEGACHIP::instruction_5xy0(s32 X, s32 Y) noexcept {
		if (mRegisterV[X] == mRegisterV[Y]) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 6 instruction branch

	void MEGACHIP::instruction_6xNN(s32 X, s32 NN) noexcept {
		::assign_cast(mRegisterV[X], NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 7 instruction branch

	void MEGACHIP::instruction_7xNN(s32 X, s32 NN) noexcept {
		::assign_cast(mRegisterV[X], mRegisterV[X] + NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 8 instruction branch

	void MEGACHIP::instruction_8xy0(s32 X, s32 Y) noexcept {
		mRegisterV[X] = mRegisterV[Y];
	}
	void MEGACHIP::instruction_8xy1(s32 X, s32 Y) noexcept {
		mRegisterV[X] |= mRegisterV[Y];
	}
	void MEGACHIP::instruction_8xy2(s32 X, s32 Y) noexcept {
		mRegisterV[X] &= mRegisterV[Y];
	}
	void MEGACHIP::instruction_8xy3(s32 X, s32 Y) noexcept {
		mRegisterV[X] ^= mRegisterV[Y];
	}
	void MEGACHIP::instruction_8xy4(s32 X, s32 Y) noexcept {
		const auto sum{ mRegisterV[X] + mRegisterV[Y] };
		::assign_cast(mRegisterV[X], sum);
		::assign_cast(mRegisterV[0xF], sum >> 8);
	}
	void MEGACHIP::instruction_8xy5(s32 X, s32 Y) noexcept {
		const bool nborrow{ mRegisterV[X] >= mRegisterV[Y] };
		::assign_cast(mRegisterV[X], mRegisterV[X] - mRegisterV[Y]);
		::assign_cast(mRegisterV[0xF], nborrow);
	}
	void MEGACHIP::instruction_8xy7(s32 X, s32 Y) noexcept {
		const bool nborrow{ mRegisterV[Y] >= mRegisterV[X] };
		::assign_cast(mRegisterV[X], mRegisterV[Y] - mRegisterV[X]);
		::assign_cast(mRegisterV[0xF], nborrow);
	}
	void MEGACHIP::instruction_8xy6(s32 X, s32  ) noexcept {
		const bool lsb{ (mRegisterV[X] & 1) == 1 };
		::assign_cast(mRegisterV[X], mRegisterV[X] >> 1);
		::assign_cast(mRegisterV[0xF], lsb);
	}
	void MEGACHIP::instruction_8xyE(s32 X, s32  ) noexcept {
		const bool msb{ (mRegisterV[X] >> 7) == 1 };
		::assign_cast(mRegisterV[X], mRegisterV[X] << 1);
		::assign_cast(mRegisterV[0xF], msb);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 9 instruction branch

	void MEGACHIP::instruction_9xy0(s32 X, s32 Y) noexcept {
		if (mRegisterV[X] != mRegisterV[Y]) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region A instruction branch

	void MEGACHIP::instruction_ANNN(s32 NNN) noexcept {
		::assign_cast(mRegisterI, NNN & 0xFFF);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region B instruction branch

	void MEGACHIP::instruction_BXNN(s32 X, s32 NNN) noexcept {
		performProgJump(NNN + mRegisterV[X]);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region C instruction branch

	void MEGACHIP::instruction_CxNN(s32 X, s32 NN) noexcept {
		::assign_cast(mRegisterV[X], RNG->next() & NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region D instruction branch

	static u32 bitBloat(u32 byte) noexcept {
		if (!byte) { return 0u; }
		byte = (byte << 4u | byte) & 0x0F0Fu;
		byte = (byte << 2u | byte) & 0x3333u;
		byte = (byte << 1u | byte) & 0x5555u;
		return  byte << 1u | byte;
	}

	bool MEGACHIP::drawSingleBytes(
		s32 originX, s32 originY,
		s32 WIDTH,   s32 DATA
	) noexcept {
		if (!DATA) { return false; }
		bool collided{ false };

		for (auto B{ 0 }; B < WIDTH; ++B) {
			const auto offsetX{ originX + B };

			if (DATA >> (WIDTH - 1 - B) & 0x1) {
				auto& pixel{ mDisplayBuffer(offsetX, originY) };
				if (!((pixel ^= 0x8) & 0x8)) { collided = true; }
			}
			if (offsetX == 0x7F) { return collided; }
		}
		return collided;
	}

	bool MEGACHIP::drawDoubleBytes(
		s32 originX, s32 originY,
		s32 WIDTH,   s32 DATA
	) noexcept {
		if (!DATA) { return false; }
		bool collided{ false };

		for (auto B{ 0 }; B < WIDTH; ++B) {
			const auto offsetX{ originX + B };

			auto& pixelHI{ mDisplayBuffer(offsetX, originY + 0) };
			auto& pixelLO{ mDisplayBuffer(offsetX, originY + 1) };

			if (DATA >> (WIDTH - 1 - B) & 0x1) {
				collided |= !!(pixelHI & 0x8);
				pixelLO = pixelHI ^= 0x8;
			} else {
				pixelLO = pixelHI;
			}
			if (offsetX == 0x7F) { return collided; }
		}
		return collided;
	}

	void MEGACHIP::instruction_DxyN(s32 X, s32 Y, s32 N) noexcept {
		if (Quirk.waitVblank) [[unlikely]]
			{ triggerInterrupt(Interrupt::FRAME); }

		if (isManualRefresh()) {
			const auto originX{ mRegisterV[X] + 0 };
			const auto originY{ mRegisterV[Y] + 0 };

			mRegisterV[0xF] = 0;

			if (!Quirk.wrapSprite && originY >= mDisplay.H) { return; }
			if (mRegisterI >= 0xF0) [[likely]] { goto paintTexture; }

			for (auto rowN{ 0 }, offsetY{ originY }; rowN < N; ++rowN)
			{
				if (Quirk.wrapSprite && offsetY >= mDisplay.H) { continue; }
				const auto octoPixelBatch{ readMemoryI(rowN) };

				for (auto colN{ 7 }, offsetX{ originX }; colN >= 0; --colN)
				{
					if (octoPixelBatch >> colN & 0x1)
					{
						auto& collideCoord{ mCollisionMap(offsetX, offsetY) };
						auto& backbufCoord{ mBackgroundBuffer(offsetX, offsetY) };

						if (collideCoord) [[unlikely]] {
							collideCoord = 0;
							backbufCoord = 0;
							mRegisterV[0xF] = 1;
						} else {
							collideCoord = 0xFF;
							backbufCoord = mFontColor[rowN];
						}
					}
					if (!Quirk.wrapSprite && offsetX == (mDisplay.W - 1)) { break; }
					else { ++offsetX &= (mDisplay.W - 1); }
				}
				if (!Quirk.wrapSprite && offsetY == (mDisplay.W - 1)) { break; }
				else { ++offsetY &= (mDisplay.W - 1); }
			}
			return;

		paintTexture:
			for (auto rowN{ 0 }, offsetY{ originY }; rowN < mTexture.H; ++rowN)
			{
				if (Quirk.wrapSprite && offsetY >= mDisplay.H) { continue; }
				const auto offsetI = rowN * mTexture.W;

				for (auto colN{ 0 }, offsetX{ originX }; colN < mTexture.W; ++colN)
				{
					if (const auto sourceColorIdx{ readMemoryI(offsetI + colN) })
					{
						auto& collideCoord{ mCollisionMap(offsetX, offsetY) };
						auto& backbufCoord{ mBackgroundBuffer(offsetX, offsetY) };

						if (collideCoord == mTexture.collide)
							[[unlikely]] { mRegisterV[0xF] = 1; }

						collideCoord = sourceColorIdx;
						backbufCoord = blendPixel(mColorPalette(sourceColorIdx), \
							backbufCoord, mBlendFunc, u8(mTexture.opacity));
					}
					if (!Quirk.wrapSprite && offsetX == (mDisplay.W - 1)) { break; }
					else { ++offsetX &= (mDisplay.W - 1); }
				}
				if (!Quirk.wrapSprite && offsetY == (mDisplay.H - 1)) { break; }
				else { ++offsetY %= mDisplay.H; }
			}
		} else {
			if (isLargerDisplay()) {
				const auto offsetX{ 8 - (mRegisterV[X] & 7) };
				const auto originX{ mRegisterV[X] & 0x78 };
				const auto originY{ mRegisterV[Y] & 0x3F };

				auto collisions{ 0 };

				if (N == 0) {
					for (auto rowN{ 0 }; rowN < 16; ++rowN) {
						const auto offsetY{ originY + rowN };

						collisions += drawSingleBytes(
							originX, offsetY, offsetX ? 24 : 16,
							(readMemoryI(2 * rowN + 0) << 8 | \
							 readMemoryI(2 * rowN + 1)) << offsetX
						);
						if (offsetY == 0x3F) { break; }
					}
				} else {
					for (auto rowN{ 0 }; rowN < N; ++rowN) {
						const auto offsetY{ originY + rowN };

						collisions += drawSingleBytes(
							originX, offsetY, offsetX ? 16 : 8,
							readMemoryI(rowN) << offsetX
						);
						if (offsetY == 0x3F) { break; }
					}
				}
				::assign_cast(mRegisterV[0xF], collisions);
			}
			else {
				const auto offsetX{ 16 - 2 * (mRegisterV[X] & 0x07) };
				const auto originX{ mRegisterV[X] * 2 & 0x70 };
				const auto originY{ mRegisterV[Y] * 2 & 0x3F };
				const auto lengthN{ N == 0 ? 16 : N };

				auto collisions{ 0 };

				for (auto rowN{ 0 }; rowN < lengthN; ++rowN) {
					const auto offsetY{ originY + rowN * 2 };

					collisions += drawDoubleBytes(
						originX, offsetY, 0x20,
						bitBloat(readMemoryI(rowN)) << offsetX
					);
					if (offsetY == 0x3E) { break; }
				}
				::assign_cast(mRegisterV[0xF], collisions != 0);
			}
		}
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region E instruction branch

	void MEGACHIP::instruction_Ex9E(s32 X) noexcept {
		if (keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
	}
	void MEGACHIP::instruction_ExA1(s32 X) noexcept {
		if (!keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region F instruction branch

	void MEGACHIP::instruction_Fx07(s32 X) noexcept {
		::assign_cast(mRegisterV[X], mDelayTimer);
	}
	void MEGACHIP::instruction_Fx0A(s32 X) noexcept {
		triggerInterrupt(Interrupt::INPUT);
		mInputReg = &mRegisterV[X];
		if (isManualRefresh()) [[unlikely]]
			{ flushAllVideoBuffers(); }
	}
	void MEGACHIP::instruction_Fx15(s32 X) noexcept {
		mDelayTimer = mRegisterV[X];
	}
	void MEGACHIP::instruction_Fx18(s32 X) noexcept {
		startVoice(mRegisterV[X] + (mRegisterV[X] == 1));
	}
	void MEGACHIP::instruction_Fx1E(s32 X) noexcept {
		mRegisterI = mRegisterI + mRegisterV[X];
	}
	void MEGACHIP::instruction_Fx29(s32 X) noexcept {
		mRegisterI = (mRegisterV[X] & 0xF) * 5 + cSmallFontOffset;
	}
	void MEGACHIP::instruction_Fx30(s32 X) noexcept {
		mRegisterI = (mRegisterV[X] & 0xF) * 10 + cLargeFontOffset;
	}
	void MEGACHIP::instruction_Fx33(s32 X) noexcept {
		const auto N__{ mRegisterV[X] * 0x51EB851Full >> 37 };
		const auto _NN{ mRegisterV[X] - N__ * 100 };
		const auto _N_{ _NN * 0xCCCDull >> 19 };
		const auto __N{ _NN - _N_ * 10 };

		writeMemoryI(N__, 0);
		writeMemoryI(_N_, 1);
		writeMemoryI(__N, 2);
	}
	void MEGACHIP::instruction_FN55(s32 N) noexcept {
		for (auto idx{ 0 }; idx <= N; ++idx)
			{ writeMemoryI(mRegisterV[idx], idx); }
	}
	void MEGACHIP::instruction_FN65(s32 N) noexcept {
		for (auto idx{ 0 }; idx <= N; ++idx)
			{ mRegisterV[idx] = readMemoryI(idx); }
	}
	void MEGACHIP::instruction_FN75(s32 N) noexcept {
		setPermaRegs(std::min(N, 7) + 1);
	}
	void MEGACHIP::instruction_FN85(s32 N) noexcept {
		getPermaRegs(std::min(N, 7) + 1);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

#endif
