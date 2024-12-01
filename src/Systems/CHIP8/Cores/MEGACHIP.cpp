/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../../Assistants/BasicVideoSpec.hpp"
#include "../../../Assistants/BasicAudioSpec.hpp"
#include "../../../Assistants/Well512.hpp"

#include "MEGACHIP.hpp"

/*==================================================================*/

MEGACHIP::MEGACHIP()
	: mDisplayBuffer{ {cScreenSizeY, cScreenSizeX} }
{
	if (getCoreState() != EmuState::FATAL) {

		copyGameToMemory(mMemoryBank.data() + cGameLoadPos);
		copyFontToMemory(mMemoryBank.data(), 0xB4);

		mForegroundBuffer.resize(false, cScreenMegaY, cScreenMegaX);
		mBackgroundBuffer.resize(false, cScreenMegaY, cScreenMegaX);
		mCollisionMap.resize(false, cScreenMegaY, cScreenMegaX);
		mColorPalette.resize(false, 1, 256);

		mCurrentPC = cStartOffset;
		mFramerate = cRefreshRate;

		prepDisplayArea(Resolution::LO);
		setNewBlendAlgorithm(BlendMode::ALPHA_BLEND);
		initializeFontColors();
	}
}

/*==================================================================*/

void MEGACHIP::instructionLoop() noexcept {

	auto cycleCount{ 0 };
	for (; cycleCount < mActiveCPF; ++cycleCount) {
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
	mTotalCycles += cycleCount;
}

void MEGACHIP::renderAudioData() {
	if (isManualRefresh()) {
		pushByteAudio(STREAM::UNIQUE);
		pushSquareTone(STREAM::BUZZER);

		BVS->setFrameColor(sBitColors[0], sBitColors[0]);
	}
	else {
		pushSquareTone(STREAM::CHANN0, cRefreshRate);
		pushSquareTone(STREAM::CHANN1, cRefreshRate);
		pushSquareTone(STREAM::CHANN2, cRefreshRate);
		pushSquareTone(STREAM::BUZZER, cRefreshRate);

		BVS->setFrameColor(sBitColors[0],
			std::accumulate(mAudioTimer.begin(), mAudioTimer.end(), 0)
			? sBitColors[1] : sBitColors[0]
		);
	}
}

void MEGACHIP::renderVideoData() {
	if (isManualRefresh()) { return; }

	BVS->modifyTexture(mDisplayBuffer[0].span(), isPixelTrailing()
		? [](const u32 pixel) noexcept {
			static constexpr u32 layer[4]{ 0xFF, 0xE7, 0x6F, 0x37 };
			const auto opacity{ layer[std::countl_zero(pixel) & 0x3] };
			return opacity << 24 | sBitColors[pixel != 0];
		}
		: [](const u32 pixel) noexcept {
			return 0xFF000000 | sBitColors[pixel >> 3];
		}
	);
}

void MEGACHIP::prepDisplayArea(const Resolution mode) {
	isDisplayLarger(mode != Resolution::LO);
	isManualRefresh(mode == Resolution::MC);

	if (isManualRefresh()) {
		setDisplayResolution(cScreenMegaX, cScreenMegaY);

		if (!BVS->setViewportDimensions(cScreenMegaX, cScreenMegaY, cResMegaMult, -2))
			[[unlikely]] { triggerInterrupt(Interrupt::ERROR); }

		Quirk.waitVblank = false;
		mActiveCPF = cInstSpeedMC;
	}
	else {
		setDisplayResolution(cScreenSizeX, cScreenSizeY);

		if (!BVS->setViewportDimensions(cScreenSizeX, cScreenSizeY, cResSizeMult, +2))
			[[unlikely]] { triggerInterrupt(Interrupt::ERROR); }

		Quirk.waitVblank = !isDisplayLarger();
		mActiveCPF = isDisplayLarger() ? cInstSpeedLo : cInstSpeedHi;
	}
};

/*==================================================================*/

void MEGACHIP::skipInstruction() noexcept {
	mCurrentPC += readMemory(mCurrentPC) == 0x1 ? 4 : 2;
}

void MEGACHIP::scrollDisplayUP(const s32 N) {
	mDisplayBuffer[0].shift(-N, 0);
}
void MEGACHIP::scrollDisplayDN(const s32 N) {
	mDisplayBuffer[0].shift(+N, 0);
}
void MEGACHIP::scrollDisplayLT() {
	mDisplayBuffer[0].shift(0, -4);
}
void MEGACHIP::scrollDisplayRT() {
	mDisplayBuffer[0].shift(0, +4);
}

/*==================================================================*/

void MEGACHIP::initializeFontColors() noexcept {
	for (auto i{ 0 }; i < 10; ++i) {
		const auto mult{ 1.0f - 0.045f * i };
		const auto R{ 0xFF * mult * 1.03f };
		const auto G{ 0xFF * mult * 1.14f };
		const auto B{ 0xFF * mult * 1.21f };

		mFontColor[i] = 0xFF000000
			| static_cast<u32>(std::min(std::round(R), 255.0f)) << 16
			| static_cast<u32>(std::min(std::round(G), 255.0f)) <<  8
			| static_cast<u32>(std::min(std::round(B), 255.0f));
	}
}

u32  MEGACHIP::blendPixel(const u32 srcPixel, const u32 dstPixel) const noexcept {
	static constexpr f32 minF{ 1.0f / 255.0f };
	struct { f32 A, R, G, B; } src{}, dst{};

	src.A =  (srcPixel >> 24) * mTexture.opacity * minF;
	if (src.A < minF) [[unlikely]] { return dstPixel; }
	src.R = ((srcPixel >> 16) & 0xFF) * minF;
	src.G = ((srcPixel >>  8) & 0xFF) * minF;
	src.B =  (srcPixel        & 0xFF) * minF;

	dst.A =  (dstPixel >> 24)         * minF;
	dst.R = ((dstPixel >> 16) & 0xFF) * minF;
	dst.G = ((dstPixel >>  8) & 0xFF) * minF;
	dst.B =  (dstPixel        & 0xFF) * minF;

	auto R{ fpBlendAlgorithm(src.R, dst.R) };
	auto G{ fpBlendAlgorithm(src.G, dst.G) };
	auto B{ fpBlendAlgorithm(src.B, dst.B) };

	if (src.A < 1.0f) {
		const f32 sW{ src.A / 1.0f };
		const f32 dW{ 1.0f - sW };

		R = dW * dst.R + sW * R;
		G = dW * dst.G + sW * G;
		B = dW * dst.B + sW * B;
	}

	return 0xFF000000
		| static_cast<u8>(std::roundf(R * 255.0f)) << 16
		| static_cast<u8>(std::roundf(G * 255.0f)) <<  8
		| static_cast<u8>(std::roundf(B * 255.0f));
}

void MEGACHIP::setNewBlendAlgorithm(const s32 mode) noexcept {
	switch (mode) {
		case BlendMode::LINEAR_DODGE:
			fpBlendAlgorithm = [](const f32 src, const f32 dst)
				noexcept { return std::min(src + dst, 1.0f); };
			break;

		case BlendMode::MULTIPLY:
			fpBlendAlgorithm = [](const f32 src, const f32 dst)
				noexcept { return src * dst; };
			break;

		default:
		case BlendMode::ALPHA_BLEND:
			fpBlendAlgorithm = [](const f32 src, const f32)
				noexcept { return src; };
			break;
	}
}

void MEGACHIP::scrapAllVideoBuffers() {
	mBackgroundBuffer.wipeAll();
	mCollisionMap.wipeAll();
}

void MEGACHIP::flushAllVideoBuffers() {
	mForegroundBuffer = mBackgroundBuffer;
	mBackgroundBuffer.wipeAll();
	mCollisionMap.wipeAll();

	BVS->modifyTexture(mForegroundBuffer.span());
}

void MEGACHIP::blendAndFlushBuffers() const {
	BVS->modifyTexture(
		mForegroundBuffer.span(),
		mBackgroundBuffer.span(),
		[this](const u32 src, const u32 dst) noexcept {
			return blendPixel(src, dst);
		}
	);
}

void MEGACHIP::resetAudioTrack() noexcept {
	mTrackPosition = mTrackStepping = 0.0;
	mTrackStartIdx = mTrackTotalLen = 0;
}

void MEGACHIP::startAudioTrack(const bool repeat) noexcept {
	mTrackTotalLen = readMemoryI(2) << 16
				   | readMemoryI(3) <<  8
				   | readMemoryI(4);

	if (!mTrackTotalLen) {
		resetAudioTrack();
		return;
	}

	mTrackStepping = readMemoryI(0) << 8 | readMemoryI(1);
	mTrackStepping = mTrackStepping / ASB->getFrequency();

	mTrackTotalLen = repeat ? -mTrackTotalLen : mTrackTotalLen;
	mTrackPosition = 0.0;
	mTrackStartIdx = mRegisterI + 6;
}

void MEGACHIP::pushByteAudio(const u32 index) noexcept {
	static const auto samplesTotal{ ASB->getSampleRate(cRefreshRate) };
	std::vector<s8> samplesBuffer(static_cast<usz>(samplesTotal));

	if (mTrackTotalLen) {
		for (auto& sample : samplesBuffer) {
			sample = static_cast<s8>((readMemory(
				mTrackStartIdx + static_cast<u32>(mTrackPosition)
			) - 128));

			if ((mTrackPosition += mTrackStepping) >= std::abs(mTrackTotalLen)) {
				if (mTrackTotalLen < 0) {
					mTrackPosition += mTrackTotalLen;
				} else {
					resetAudioTrack();
					break;
				}
			}
		}
	}
	ASB->pushAudioData(index, samplesBuffer);
}

void MEGACHIP::scrollBuffersUP(const s32 N) {
	mForegroundBuffer.shift(-N, 0);
	blendAndFlushBuffers();
}
void MEGACHIP::scrollBuffersDN(const s32 N) {
	mForegroundBuffer.shift(+N, 0);
	blendAndFlushBuffers();
}
void MEGACHIP::scrollBuffersLT() {
	mForegroundBuffer.shift(0, -4);
	blendAndFlushBuffers();
}
void MEGACHIP::scrollBuffersRT() {
	mForegroundBuffer.shift(0, +4);
	blendAndFlushBuffers();
}

/*==================================================================*/
	#pragma region 0 instruction branch

	void MEGACHIP::instruction_00BN(const s32 N) noexcept {
		if (isManualRefresh()) {
			scrollBuffersUP(N);
		} else {
			scrollDisplayUP(N);
		}
	}
	void MEGACHIP::instruction_00CN(const s32 N) noexcept {
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
			mDisplayBuffer[0].wipeAll();
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

		ASB->resumeStream(STREAM::CHANN1);
		ASB->resumeStream(STREAM::CHANN2);

		resetAudioTrack();
		flushAllVideoBuffers();

		prepDisplayArea(Resolution::LO);
	}
	void MEGACHIP::instruction_0011() noexcept {
		triggerInterrupt(Interrupt::FRAME);

		ASB->pauseStream(STREAM::CHANN1);
		ASB->pauseStream(STREAM::CHANN2);

		resetAudioTrack();
		scrapAllVideoBuffers();

		prepDisplayArea(Resolution::MC);
	}
	void MEGACHIP::instruction_01NN(const s32 NN) noexcept {
		mRegisterI = (NN << 16) | NNNN();
		nextInstruction();
	}
	void MEGACHIP::instruction_02NN(const s32 NN) noexcept {
		for (auto pos{ 0 }, offset{ 0 }; pos < NN; offset += 4) {
			mColorPalette.at_raw(++pos)
				= readMemoryI(offset + 0) << 24
				| readMemoryI(offset + 1) << 16
				| readMemoryI(offset + 2) <<  8
				| readMemoryI(offset + 3);
		}
	}
	void MEGACHIP::instruction_03NN(const s32 NN) noexcept {
		mTexture.W = NN ? NN : 256;
	}
	void MEGACHIP::instruction_04NN(const s32 NN) noexcept {
		mTexture.H = NN ? NN : 256;
	}
	void MEGACHIP::instruction_05NN(const s32 NN) noexcept {
		BVS->setViewportOpacity(NN);
	}
	void MEGACHIP::instruction_060N(const s32 N) noexcept {
		startAudioTrack(N == 0);
	}
	void MEGACHIP::instruction_0700() noexcept {
		resetAudioTrack();
	}
	void MEGACHIP::instruction_080N(const s32 N) noexcept {
		static constexpr float opacity[]{ 1.0f, 0.25f, 0.50f, 0.75f };
		mTexture.opacity = opacity[N > 3 ? 0 : N];
		setNewBlendAlgorithm(N);
	}
	void MEGACHIP::instruction_09NN(const s32 NN) noexcept {
		mTexture.collide = NN;
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 1 instruction branch

	void MEGACHIP::instruction_1NNN(const s32 NNN) noexcept {
		performProgJump(NNN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 2 instruction branch

	void MEGACHIP::instruction_2NNN(const s32 NNN) noexcept {
		mStackBank[mStackTop++ & 0xF] = mCurrentPC;
		performProgJump(NNN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 3 instruction branch

	void MEGACHIP::instruction_3xNN(const s32 X, const s32 NN) noexcept {
		if (mRegisterV[X] == NN) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 4 instruction branch

	void MEGACHIP::instruction_4xNN(const s32 X, const s32 NN) noexcept {
		if (mRegisterV[X] != NN) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 5 instruction branch

	void MEGACHIP::instruction_5xy0(const s32 X, const s32 Y) noexcept {
		if (mRegisterV[X] == mRegisterV[Y]) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 6 instruction branch

	void MEGACHIP::instruction_6xNN(const s32 X, const s32 NN) noexcept {
		mRegisterV[X] = static_cast<u8>(NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 7 instruction branch

	void MEGACHIP::instruction_7xNN(const s32 X, const s32 NN) noexcept {
		mRegisterV[X] += static_cast<u8>(NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 8 instruction branch

	void MEGACHIP::instruction_8xy0(const s32 X, const s32 Y) noexcept {
		mRegisterV[X] = mRegisterV[Y];
	}
	void MEGACHIP::instruction_8xy1(const s32 X, const s32 Y) noexcept {
		mRegisterV[X] |= mRegisterV[Y];
	}
	void MEGACHIP::instruction_8xy2(const s32 X, const s32 Y) noexcept {
		mRegisterV[X] &= mRegisterV[Y];
	}
	void MEGACHIP::instruction_8xy3(const s32 X, const s32 Y) noexcept {
		mRegisterV[X] ^= mRegisterV[Y];
	}
	void MEGACHIP::instruction_8xy4(const s32 X, const s32 Y) noexcept {
		const auto sum{ mRegisterV[X] + mRegisterV[Y] };
		mRegisterV[X]   = static_cast<u8>(sum);
		mRegisterV[0xF] = static_cast<u8>(sum >> 8);
	}
	void MEGACHIP::instruction_8xy5(const s32 X, const s32 Y) noexcept {
		const bool nborrow{ mRegisterV[X] >= mRegisterV[Y] };
		mRegisterV[X]   = static_cast<u8>(mRegisterV[X] - mRegisterV[Y]);
		mRegisterV[0xF] = static_cast<u8>(nborrow);
	}
	void MEGACHIP::instruction_8xy7(const s32 X, const s32 Y) noexcept {
		const bool nborrow{ mRegisterV[Y] >= mRegisterV[X] };
		mRegisterV[X]   = static_cast<u8>(mRegisterV[Y] - mRegisterV[X]);
		mRegisterV[0xF] = static_cast<u8>(nborrow);
	}
	void MEGACHIP::instruction_8xy6(const s32 X, const s32  ) noexcept {
		const bool lsb{ (mRegisterV[X] & 1) == 1 };
		mRegisterV[X]   = static_cast<u8>(mRegisterV[X] >> 1);
		mRegisterV[0xF] = static_cast<u8>(lsb);
	}
	void MEGACHIP::instruction_8xyE(const s32 X, const s32  ) noexcept {
		const bool msb{ (mRegisterV[X] >> 7) == 1 };
		mRegisterV[X]   = static_cast<u8>(mRegisterV[X] << 1);
		mRegisterV[0xF] = static_cast<u8>(msb);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 9 instruction branch

	void MEGACHIP::instruction_9xy0(const s32 X, const s32 Y) noexcept {
		if (mRegisterV[X] != mRegisterV[Y]) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region A instruction branch

	void MEGACHIP::instruction_ANNN(const s32 NNN) noexcept {
		mRegisterI = NNN & 0xFFF;
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region B instruction branch

	void MEGACHIP::instruction_BXNN(const s32 X, const s32 NNN) noexcept {
		performProgJump(NNN + mRegisterV[X]);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region C instruction branch

	void MEGACHIP::instruction_CxNN(const s32 X, const s32 NN) noexcept {
		mRegisterV[X] = Wrand->get<u8>() & NN;
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
		const s32 originX, const s32 originY,
		const s32 WIDTH,   const s32 DATA
	) noexcept {
		if (!DATA) { return false; }
		bool collided{ false };

		for (auto B{ 0 }; B < WIDTH; ++B) {
			const auto offsetX{ originX + B };

			if (DATA >> (WIDTH - 1 - B) & 0x1) {
				auto& pixel{ mDisplayBuffer[0].at_raw(originY, offsetX) };
				if (!((pixel ^= 0x8) & 0x8)) { collided = true; }
			}
			if (offsetX == 0x7F) { return collided; }
		}
		return collided;
	}

	bool MEGACHIP::drawDoubleBytes(
		const s32 originX, const s32 originY,
		const s32 WIDTH,   const s32 DATA
	) noexcept {
		if (!DATA) { return false; }
		bool collided{ false };

		for (auto B{ 0 }; B < WIDTH; ++B) {
			const auto offsetX{ originX + B };

			auto& pixelHI{ mDisplayBuffer[0].at_raw(originY + 0, offsetX) };
			auto& pixelLO{ mDisplayBuffer[0].at_raw(originY + 1, offsetX) };

			if (DATA >> (WIDTH - 1 - B) & 0x1) {
				if (pixelHI & 0x8) { collided = true; }
				pixelLO = pixelHI ^= 0x8;
			} else {
				pixelLO = pixelHI;
			}
			if (offsetX == 0x7F) { return collided; }
		}
		return collided;
	}

	void MEGACHIP::instruction_DxyN(const s32 X, const s32 Y, const s32 N) noexcept {
		if (Quirk.waitVblank) [[unlikely]]
			{ triggerInterrupt(Interrupt::FRAME); }

		if (isManualRefresh()) {
			const auto originX{ mRegisterV[X] + 0 };
			const auto originY{ mRegisterV[Y] + 0 };

			mRegisterV[0xF] = 0;

			if (!Quirk.wrapSprite && originY >= mDisplayH) { return; }
			if (mRegisterI >= 0xF0) [[likely]] { goto paintTexture; }

			for (auto rowN{ 0 }, offsetY{ originY }; rowN < N; ++rowN)
			{
				if (Quirk.wrapSprite && offsetY >= mDisplayH) { continue; }
				const auto octoPixelBatch{ readMemoryI(rowN) };

				for (auto colN{ 7 }, offsetX{ originX }; colN >= 0; --colN)
				{
					if (octoPixelBatch >> colN & 0x1)
					{
						auto& collideCoord{ mCollisionMap.at_raw(offsetY, offsetX) };
						auto& backbufCoord{ mBackgroundBuffer.at_raw(offsetY, offsetX) };

						if (collideCoord) [[unlikely]] {
							collideCoord = 0;
							backbufCoord = 0;
							mRegisterV[0xF] = 1;
						} else {
							collideCoord = 0xFF;
							backbufCoord = mFontColor[rowN];
						}
					}
					if (!Quirk.wrapSprite && offsetX == mDisplayWb) { break; }
					else { ++offsetX &= mDisplayWb; }
				}
				if (!Quirk.wrapSprite && offsetY == mDisplayWb) { break; }
				else { ++offsetY &= mDisplayWb; }
			}
			return;

		paintTexture:
			for (auto rowN{ 0 }, offsetY{ originY }; rowN < mTexture.H; ++rowN)
			{
				if (Quirk.wrapSprite && offsetY >= mDisplayH) { continue; }
				auto I = rowN * mTexture.W;

				for (auto colN{ 0 }, offsetX{ originX }; colN < mTexture.W; ++colN, ++I)
				{
					if (const auto sourceColorIdx{ readMemoryI(I) }; sourceColorIdx)
					{
						auto& collideCoord{ mCollisionMap.at_raw(offsetY, offsetX) };
						auto& backbufCoord{ mBackgroundBuffer.at_raw(offsetY, offsetX) };

						if (collideCoord == mTexture.collide)
							[[unlikely]] { mRegisterV[0xF] = 1; }

						collideCoord = sourceColorIdx;
						backbufCoord = blendPixel(
							mColorPalette.at_raw(sourceColorIdx),
							backbufCoord);
					}
					if (!Quirk.wrapSprite && offsetX == mDisplayWb) { break; }
					else { ++offsetX &= mDisplayWb; }
				}
				if (!Quirk.wrapSprite && offsetY == mDisplayHb) { break; }
				else { ++offsetY %= mDisplayH; }
			}
		} else {
			if (isDisplayLarger()) {
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
				mRegisterV[0xF] = static_cast<u8>(collisions);
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
				mRegisterV[0xF] = static_cast<u8>(collisions != 0);
			}
		}
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region E instruction branch

	void MEGACHIP::instruction_Ex9E(const s32 X) noexcept {
		if (keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
	}
	void MEGACHIP::instruction_ExA1(const s32 X) noexcept {
		if (!keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region F instruction branch

	void MEGACHIP::instruction_Fx07(const s32 X) noexcept {
		mRegisterV[X] = static_cast<u8>(mDelayTimer);
	}
	void MEGACHIP::instruction_Fx0A(const s32 X) noexcept {
		triggerInterrupt(Interrupt::INPUT);
		mInputReg = &mRegisterV[X];
		if (isManualRefresh()) [[unlikely]]
			{ flushAllVideoBuffers(); }
	}
	void MEGACHIP::instruction_Fx15(const s32 X) noexcept {
		mDelayTimer = mRegisterV[X];
	}
	void MEGACHIP::instruction_Fx18(const s32 X) noexcept {
		startAudio(mRegisterV[X] + (mRegisterV[X] == 1));
	}
	void MEGACHIP::instruction_Fx1E(const s32 X) noexcept {
		mRegisterI = mRegisterI + mRegisterV[X];
	}
	void MEGACHIP::instruction_Fx29(const s32 X) noexcept {
		mRegisterI = (mRegisterV[X] & 0xF) * 5;
	}
	void MEGACHIP::instruction_Fx30(const s32 X) noexcept {
		mRegisterI = (mRegisterV[X] & 0xF) * 10 + 80;
	}
	void MEGACHIP::instruction_Fx33(const s32 X) noexcept {
		writeMemoryI(mRegisterV[X] / 100,     0);
		writeMemoryI(mRegisterV[X] / 10 % 10, 1);
		writeMemoryI(mRegisterV[X]      % 10, 2);
	}
	void MEGACHIP::instruction_FN55(const s32 N) noexcept {
		for (auto idx{ 0 }; idx <= N; ++idx)
			{ writeMemoryI(mRegisterV[idx], idx); }
	}
	void MEGACHIP::instruction_FN65(const s32 N) noexcept {
		for (auto idx{ 0 }; idx <= N; ++idx)
			{ mRegisterV[idx] = readMemoryI(idx); }
	}
	void MEGACHIP::instruction_FN75(const s32 N) noexcept {
		if (!setPermaRegs(std::min(N, 7) + 1)) [[unlikely]]
			{ triggerCritError("Error :: Failed writing persistent registers!"); }
	}
	void MEGACHIP::instruction_FN85(const s32 N) noexcept {
		if (!getPermaRegs(std::min(N, 7) + 1)) [[unlikely]]
			{ triggerCritError("Error :: Failed reading persistent registers!"); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
