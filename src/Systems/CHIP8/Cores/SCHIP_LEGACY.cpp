/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "SCHIP_LEGACY.hpp"
#ifdef ENABLE_SCHIP_LEGACY

#include "../../../Assistants/BasicVideoSpec.hpp"
#include "../../../Assistants/BasicAudioSpec.hpp"
#include "../../../Assistants/Well512.hpp"
#include "../../CoreRegistry.hpp"

REGISTER_CORE(SCHIP_LEGACY, ".sc8")

/*==================================================================*/

SCHIP_LEGACY::SCHIP_LEGACY()
	: mDisplayBuffer{ {cDisplayResW, cDisplayResH} }
{
	::generate_n(mMemoryBank, 0, cTotalMemory, []() { return RNG->next<u8>(); });
	::fill_n(mMemoryBank, cTotalMemory, cSafezoneOOB, 0xFF);

	copyGameToMemory(mMemoryBank.data() + cGameLoadPos);
	copyFontToMemory(mMemoryBank.data(), 0xB4);

	setDisplayResolution(cDisplayResW, cDisplayResH);
	setViewportSizes(true, cDisplayResW, cDisplayResH, cResSizeMult, 2);
	setSystemFramerate(cRefreshRate);

	mCurrentPC = cStartOffset;

	prepDisplayArea(Resolution::LO);
}

/*==================================================================*/

void SCHIP_LEGACY::instructionLoop() noexcept {

	auto cycleCount{ 0 };
	for (; cycleCount < mTargetCPF; ++cycleCount) {
		const auto HI{ mMemoryBank[mCurrentPC + 0u] };
		const auto LO{ mMemoryBank[mCurrentPC + 1u] };
		nextInstruction();

		switch (HI >> 4) {
			case 0x0:
				switch (HI << 8 | LO) {
					             case 0x00C1: case 0x00C2: case 0x00C3:
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
	mElapsedCycles += cycleCount;
}

void SCHIP_LEGACY::renderAudioData() {
	pushSquareTone(STREAM::CHANN0);
	pushSquareTone(STREAM::CHANN1);
	pushSquareTone(STREAM::CHANN2);
	pushSquareTone(STREAM::BUZZER);

	setDisplayBorderColor(sBitColors[!!std::accumulate(mAudioTimer.begin(), mAudioTimer.end(), 0)]);
}

void SCHIP_LEGACY::renderVideoData() {
	BVS->displayBuffer.write(mDisplayBuffer[0], isUsingPixelTrails()
		? [](u32 pixel) noexcept {
			static constexpr u32 layer[4]{ 0xFF, 0xE7, 0x6F, 0x37 };
			const auto opacity{ layer[std::countl_zero(pixel) & 0x3] };
			return opacity | sBitColors[pixel != 0];
		}
		: [](u32 pixel) noexcept {
			return 0xFF | sBitColors[pixel >> 3];
		}
	);

	std::for_each(EXEC_POLICY(unseq)
		mDisplayBuffer[0].begin(),
		mDisplayBuffer[0].end(),
		[](auto& pixel) noexcept
			{ ::assign_cast(pixel, (pixel & 0x8) | (pixel >> 1)); }
	);
}

void SCHIP_LEGACY::prepDisplayArea(const Resolution mode) {
	isLargerDisplay(mode != Resolution::LO);

	Quirk.waitVblank = !isLargerDisplay();
	mTargetCPF = isLargerDisplay() ? cInstSpeedLo : cInstSpeedHi;
};

/*==================================================================*/

void SCHIP_LEGACY::scrollDisplayDN(s32 N) {
	mDisplayBuffer[0].shift(0, +N);
}
void SCHIP_LEGACY::scrollDisplayLT() {
	mDisplayBuffer[0].shift(-4, 0);
}
void SCHIP_LEGACY::scrollDisplayRT() {
	mDisplayBuffer[0].shift(+4, 0);
}

/*==================================================================*/
	#pragma region 0 instruction branch

	void SCHIP_LEGACY::instruction_00CN(s32 N) noexcept {
		scrollDisplayDN(N);
	}
	void SCHIP_LEGACY::instruction_00E0() noexcept {
		triggerInterrupt(Interrupt::FRAME);
		mDisplayBuffer[0].initialize();
	}
	void SCHIP_LEGACY::instruction_00EE() noexcept {
		mCurrentPC = mStackBank[--mStackTop & 0xF];
	}
	void SCHIP_LEGACY::instruction_00FB() noexcept {
		scrollDisplayRT();
	}
	void SCHIP_LEGACY::instruction_00FC() noexcept {
		scrollDisplayLT();
	}
	void SCHIP_LEGACY::instruction_00FD() noexcept {
		triggerInterrupt(Interrupt::SOUND);
	}
	void SCHIP_LEGACY::instruction_00FE() noexcept {
		triggerInterrupt(Interrupt::FRAME);
		prepDisplayArea(Resolution::LO);
	}
	void SCHIP_LEGACY::instruction_00FF() noexcept {
		triggerInterrupt(Interrupt::FRAME);
		prepDisplayArea(Resolution::HI);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 1 instruction branch

	void SCHIP_LEGACY::instruction_1NNN(s32 NNN) noexcept {
		performProgJump(NNN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 2 instruction branch

	void SCHIP_LEGACY::instruction_2NNN(s32 NNN) noexcept {
		mStackBank[mStackTop++ & 0xF] = mCurrentPC;
		performProgJump(NNN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 3 instruction branch

	void SCHIP_LEGACY::instruction_3xNN(s32 X, s32 NN) noexcept {
		if (mRegisterV[X] == NN) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 4 instruction branch

	void SCHIP_LEGACY::instruction_4xNN(s32 X, s32 NN) noexcept {
		if (mRegisterV[X] != NN) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 5 instruction branch

	void SCHIP_LEGACY::instruction_5xy0(s32 X, s32 Y) noexcept {
		if (mRegisterV[X] == mRegisterV[Y]) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 6 instruction branch

	void SCHIP_LEGACY::instruction_6xNN(s32 X, s32 NN) noexcept {
		::assign_cast(mRegisterV[X], NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 7 instruction branch

	void SCHIP_LEGACY::instruction_7xNN(s32 X, s32 NN) noexcept {
		::assign_cast(mRegisterV[X], mRegisterV[X] + NN);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 8 instruction branch

	void SCHIP_LEGACY::instruction_8xy0(s32 X, s32 Y) noexcept {
		mRegisterV[X] = mRegisterV[Y];
	}
	void SCHIP_LEGACY::instruction_8xy1(s32 X, s32 Y) noexcept {
		mRegisterV[X] |= mRegisterV[Y];
	}
	void SCHIP_LEGACY::instruction_8xy2(s32 X, s32 Y) noexcept {
		mRegisterV[X] &= mRegisterV[Y];
	}
	void SCHIP_LEGACY::instruction_8xy3(s32 X, s32 Y) noexcept {
		mRegisterV[X] ^= mRegisterV[Y];
	}
	void SCHIP_LEGACY::instruction_8xy4(s32 X, s32 Y) noexcept {
		const auto sum{ mRegisterV[X] + mRegisterV[Y] };
		::assign_cast(mRegisterV[X], sum);
		::assign_cast(mRegisterV[0xF], sum >> 8);
	}
	void SCHIP_LEGACY::instruction_8xy5(s32 X, s32 Y) noexcept {
		const bool nborrow{ mRegisterV[X] >= mRegisterV[Y] };
		::assign_cast(mRegisterV[X], mRegisterV[X] - mRegisterV[Y]);
		::assign_cast(mRegisterV[0xF], nborrow);
	}
	void SCHIP_LEGACY::instruction_8xy7(s32 X, s32 Y) noexcept {
		const bool nborrow{ mRegisterV[Y] >= mRegisterV[X] };
		::assign_cast(mRegisterV[X], mRegisterV[Y] - mRegisterV[X]);
		::assign_cast(mRegisterV[0xF], nborrow);
	}
	void SCHIP_LEGACY::instruction_8xy6(s32 X, s32  ) noexcept {
		const bool lsb{ (mRegisterV[X] & 1) == 1 };
		::assign_cast(mRegisterV[X], mRegisterV[X] >> 1);
		::assign_cast(mRegisterV[0xF], lsb);
	}
	void SCHIP_LEGACY::instruction_8xyE(s32 X, s32  ) noexcept {
		const bool msb{ (mRegisterV[X] >> 7) == 1 };
		::assign_cast(mRegisterV[X], mRegisterV[X] << 1);
		::assign_cast(mRegisterV[0xF], msb);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 9 instruction branch

	void SCHIP_LEGACY::instruction_9xy0(s32 X, s32 Y) noexcept {
		if (mRegisterV[X] != mRegisterV[Y]) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region A instruction branch

	void SCHIP_LEGACY::instruction_ANNN(s32 NNN) noexcept {
		mRegisterI = NNN & 0xFFF;
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region B instruction branch

	void SCHIP_LEGACY::instruction_BXNN(s32 X, s32 NNN) noexcept {
		performProgJump(NNN + mRegisterV[X]);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region C instruction branch

	void SCHIP_LEGACY::instruction_CxNN(s32 X, s32 NN) noexcept {
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

	bool SCHIP_LEGACY::drawSingleBytes(
		s32 originX, s32 originY,
		s32 WIDTH,   s32 DATA
	) noexcept {
		if (!DATA) { return false; }
		bool collided{ false };

		for (auto B{ 0 }; B < WIDTH; ++B) {
			const auto offsetX{ originX + B };

			if (DATA >> (WIDTH - 1 - B) & 0x1) {
				auto& pixel{ mDisplayBuffer[0](offsetX, originY) };
				if (!((pixel ^= 0x8) & 0x8)) { collided = true; }
			}
			if (offsetX == cDisplayResW - 1) { return collided; }
		}
		return collided;
	}

	bool SCHIP_LEGACY::drawDoubleBytes(
		s32 originX, s32 originY,
		s32 WIDTH,   s32 DATA
	) noexcept {
		if (!DATA) { return false; }
		bool collided{ false };

		for (auto B{ 0 }; B < WIDTH; ++B) {
			const auto offsetX{ originX + B };

			auto& pixelHI{ mDisplayBuffer[0](offsetX, originY + 0) };
			auto& pixelLO{ mDisplayBuffer[0](offsetX, originY + 1) };

			if (DATA >> (WIDTH - 1 - B) & 0x1) {
				if (pixelHI & 0x8) { collided = true; }
				pixelLO = pixelHI ^= 0x8;
			} else {
				pixelLO = pixelHI;
			}
			if (offsetX == cDisplayResW - 1) { return collided; }
		}
		return collided;
	}

	void SCHIP_LEGACY::instruction_DxyN(s32 X, s32 Y, s32 N) noexcept {
		if (Quirk.waitVblank) [[unlikely]]
			{ triggerInterrupt(Interrupt::FRAME); }

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
						(readMemoryI(2 * rowN + 0)  << 8 | \
						 readMemoryI(2 * rowN + 1)) << offsetX
					);
					if (offsetY == cDisplayResH - 1) { break; }
				}
			} else {
				for (auto rowN{ 0 }; rowN < N; ++rowN) {
					const auto offsetY{ originY + rowN };

					collisions += drawSingleBytes(
						originX, offsetY, offsetX ? 16 : 8,
						readMemoryI(rowN) << offsetX
					);
					if (offsetY == cDisplayResH - 1) { break; }
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

				if (offsetY == cDisplayResH - 2) { break; }
			}
			::assign_cast(mRegisterV[0xF], collisions != 0);
		}
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region E instruction branch

	void SCHIP_LEGACY::instruction_Ex9E(s32 X) noexcept {
		if (keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
	}
	void SCHIP_LEGACY::instruction_ExA1(s32 X) noexcept {
		if (!keyHeld_P1(mRegisterV[X])) { skipInstruction(); }
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region F instruction branch

	void SCHIP_LEGACY::instruction_Fx07(s32 X) noexcept {
		::assign_cast(mRegisterV[X], mDelayTimer);
	}
	void SCHIP_LEGACY::instruction_Fx0A(s32 X) noexcept {
		triggerInterrupt(Interrupt::INPUT);
		mInputReg = &mRegisterV[X];
	}
	void SCHIP_LEGACY::instruction_Fx15(s32 X) noexcept {
		mDelayTimer = mRegisterV[X];
	}
	void SCHIP_LEGACY::instruction_Fx18(s32 X) noexcept {
		startAudio(mRegisterV[X] + (mRegisterV[X] == 1));
	}
	void SCHIP_LEGACY::instruction_Fx1E(s32 X) noexcept {
		mRegisterI = (mRegisterI + mRegisterV[X]) & 0xFFF;
	}
	void SCHIP_LEGACY::instruction_Fx29(s32 X) noexcept {
		mRegisterI = (mRegisterV[X] & 0xF) * 5;
	}
	void SCHIP_LEGACY::instruction_Fx30(s32 X) noexcept {
		mRegisterI = (mRegisterV[X] & 0xF) * 10 + 80;
	}
	void SCHIP_LEGACY::instruction_Fx33(s32 X) noexcept {
		const auto N__{ mRegisterV[X] * 0x51EB851Full >> 37 };
		const auto _NN{ mRegisterV[X] - N__ * 100 };
		const auto _N_{ _NN * 0xCCCDull >> 19 };
		const auto __N{ _NN - _N_ * 10 };

		writeMemoryI(N__, 0);
		writeMemoryI(_N_, 1);
		writeMemoryI(__N, 2);
	}
	void SCHIP_LEGACY::instruction_FN55(s32 N) noexcept {
		SUGGEST_VECTORIZABLE_LOOP
		for (auto idx{ 0 }; idx <= N; ++idx) { writeMemoryI(mRegisterV[idx], idx); }
		mRegisterI = Quirk.idxRegMinus ? (mRegisterI + N) & 0xFFF : mRegisterI;
		//if (Quirk.idxRegMinus) [[likely]] { mRegisterI = (mRegisterI + N) & 0xFFF; }
	}
	void SCHIP_LEGACY::instruction_FN65(s32 N) noexcept {
		SUGGEST_VECTORIZABLE_LOOP
		for (auto idx{ 0 }; idx <= N; ++idx) { mRegisterV[idx] = readMemoryI(idx); }
		mRegisterI = Quirk.idxRegMinus ? (mRegisterI + N) & 0xFFF : mRegisterI;
		//if (Quirk.idxRegMinus) [[likely]] { mRegisterI = (mRegisterI + N) & 0xFFF; }
	}
	void SCHIP_LEGACY::instruction_FN75(s32 N) noexcept {
		setPermaRegs(std::min(N, 7) + 1);
	}
	void SCHIP_LEGACY::instruction_FN85(s32 N) noexcept {
		getPermaRegs(std::min(N, 7) + 1);
	}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
	
#endif
