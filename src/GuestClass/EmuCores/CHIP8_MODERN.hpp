/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "EmuCores.hpp"

class CHIP8_MODERN final : public EmuCores {
	static constexpr u32 cTotalMemory{ 0x1000u };
	static constexpr u32 cGameLoadPos{ 0x0200u };
	static constexpr u32 cStartOffset{ 0x0200u };
	static constexpr f32 cRefreshRate{ 60.000f };
	static constexpr s32 cInstSpeedHi{     30  };
	static constexpr s32 cInstSpeedLo{     11  };

public:
	static constexpr bool testGameSize(const usz size) noexcept {
		return size + cGameLoadPos <= cTotalMemory;
	}

public:
	explicit CHIP8_MODERN(
		HomeDirManager&,
		BasicVideoSpec&,
		BasicAudioSpec&
	) noexcept;
	~CHIP8_MODERN() noexcept;

	void processFrame() override;


private:
	u8  mRegisterV[16]{};
	u16 mStackBank[16]{};

	f32  mWavePhase{};
	f32  mAudioTone{};

	f32  calcAudioTone() const;
	void renderAudioData();

	u8  mDelayTimer{};
	u8  mSoundTimer{};

	u16 mProgCounter{};

	u8  mInputReg{};
	u8  mStackTop{};
	u16 mRegisterI{};

	std::array<u8, cTotalMemory>
		mMemoryBank{};
	
	std::array<u8, 2048>
		mDisplayBuffer{};

	bool in_range(const usz pos) const noexcept { return pos < mMemoryBank.size(); }

	//auto& VX() noexcept { return mRegisterV[(mInstruction >> 8) & 0xF]; }

	// Write memory at given index using given value
	void writeMemory(const usz value, const usz pos) noexcept {
		//mMemoryBank[pos & mMemoryBank.size() - 1] = static_cast<u8>(value);
		if (in_range(pos)) { mMemoryBank[pos] = static_cast<u8>(value); }
	}
	// Write memory at saved index using given value
	void writeMemoryI(const usz value, const usz pos) noexcept {
		//mMemoryBank[mRegisterI + pos & mMemoryBank.size() - 1] = static_cast<u8>(value);
		if (in_range(mRegisterI + pos)) { mMemoryBank[mRegisterI + pos] = static_cast<u8>(value); }
	}
	// Write memory at saved index using given value
	void writeMemoryI(const usz value) noexcept {
		//mMemoryBank[mRegisterI & mMemoryBank.size() - 1] = static_cast<u8>(value);
		if (in_range(mRegisterI)) { mMemoryBank[mRegisterI] = static_cast<u8>(value); }
	}

	// Read memory at given index
	auto readMemory(const usz pos) const noexcept -> u8 {
		return (in_range(pos)) ? mMemoryBank[pos] : 0;
		//return mMemoryBank[pos & mMemoryBank.size() - 1];
	}
	// Read memory at saved index
	auto readMemoryI(const usz pos) const noexcept -> u8 {
		return (in_range(mRegisterI + pos)) ? mMemoryBank[mRegisterI + pos] : 0;
		//return mMemoryBank[mRegisterI + pos & mMemoryBank.size() - 1];
	}
	// Read memory at saved index
	auto readMemoryI() const noexcept -> u8 {
		return (in_range(mRegisterI)) ? mMemoryBank[mRegisterI] : 0;
		//return mMemoryBank[mRegisterI & mMemoryBank.size() - 1];
	}

private:
	void initPlatform();

	void renderToTexture();

	void instructionLoop();

	void handlePreFrameInterrupt() noexcept;
	void handleEndFrameInterrupt() noexcept;

	void jumpProgramTo(s32) noexcept;

/*==================================================================*/
	#pragma region 0 instruction branch
/*==================================================================*/

	// 00E0 - erase whole display
	void instruction_00E0() {
		if (Quirk.waitVblank) [[unlikely]]
			{ setInterrupt(Interrupt::FRAME); }
		std::fill(
			std::execution::unseq,
			mDisplayBuffer.begin(),
			mDisplayBuffer.end(),
			u8()
		);
	}
	// 00EE - return from subroutine
	void instruction_00EE() {
		mProgCounter = mStackBank[--mStackTop & 0xF];
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 1 instruction branch
/*==================================================================*/

	// 1NNN - jump to NNN
	void instruction_1NNN(const s32 NNN) {
		jumpProgramTo(NNN);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 2 instruction branch
/*==================================================================*/

	// 2NNN - call subroutine at NNN
	void instruction_2NNN(const s32 NNN) {
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
	void instruction_3xNN(const s32 X, const s32 NN) {
		if (mRegisterV[X] == NN) { mProgCounter += 2; }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 4 instruction branch
/*==================================================================*/

	// 4XNN - skip next instruction if VX != NN
	void instruction_4xNN(const s32 X, const s32 NN) {
		if (mRegisterV[X] != NN) { mProgCounter += 2; }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 5 instruction branch
/*==================================================================*/

	// 5XY0 - skip next instruction if VX == VY
	void instruction_5xy0(const s32 X, const s32 Y) {
		if (mRegisterV[X] == mRegisterV[Y]) { mProgCounter += 2; }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 6 instruction branch
/*==================================================================*/

	// 6XNN - set VX = NN
	void instruction_6xNN(const s32 X, const s32 NN) {
		mRegisterV[X] = static_cast<u8>(NN);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 7 instruction branch
/*==================================================================*/

	// 7XNN - set VX = VX + NN
	void instruction_7xNN(const s32 X, const s32 NN) {
		mRegisterV[X] += static_cast<u8>(NN);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 8 instruction branch
/*==================================================================*/

	// 8XY0 - set VX = VY
	void instruction_8xy0(const s32 X, const s32 Y) {
		mRegisterV[X] = mRegisterV[Y];
	}
	// 8XY1 - set VX = VX | VY
	void instruction_8xy1(const s32 X, const s32 Y) {
		mRegisterV[X] |= mRegisterV[Y];
	}
	// 8XY2 - set VX = VX & VY
	void instruction_8xy2(const s32 X, const s32 Y) {
		mRegisterV[X] &= mRegisterV[Y];
	}
	// 8XY3 - set VX = VX ^ VY
	void instruction_8xy3(const s32 X, const s32 Y) {
		mRegisterV[X] ^= mRegisterV[Y];
	}
	// 8XY4 - set VX = VX + VY, VF = carry
	void instruction_8xy4(const s32 X, const s32 Y) {
		const auto sum{ mRegisterV[X] + mRegisterV[Y] };
		mRegisterV[X]   = static_cast<u8>(sum);
		mRegisterV[0xF] = static_cast<u8>(sum >> 8);
	}
	// 8XY5 - set VX = VX - VY, VF = !borrow
	void instruction_8xy5(const s32 X, const s32 Y) {
		const bool borrow{ mRegisterV[X] < mRegisterV[Y] };
		mRegisterV[X]   = mRegisterV[X] - mRegisterV[Y];
		mRegisterV[0xF] = !borrow;
	}
	// 8XY7 - set VX = VY - VX, VF = !borrow
	void instruction_8xy7(const s32 X, const s32 Y) {
		const bool borrow{ mRegisterV[Y] < mRegisterV[X] };
		mRegisterV[X]   = mRegisterV[Y] - mRegisterV[X];
		mRegisterV[0xF] = !borrow;
	}
	// 8XY6 - set VX = VY >> 1, VF = carry
	void instruction_8xy6(const s32 X, const s32 Y) {
		if (!Quirk.shiftVX) mRegisterV[X] = mRegisterV[Y];
		const bool lsb{ (mRegisterV[X] & 1) == 1 };
		mRegisterV[X]   = mRegisterV[X] >> 1;
		mRegisterV[0xF] = lsb;
	}
	// 8XYE - set VX = VY << 1, VF = carry
	void instruction_8xyE(const s32 X, const s32 Y) {
		if (!Quirk.shiftVX) mRegisterV[X] = mRegisterV[Y];
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
	void instruction_9xy0(const s32 X, const s32 Y) {
		if (mRegisterV[X] != mRegisterV[Y]) { mProgCounter += 2; }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region A instruction branch
/*==================================================================*/

	// ANNN - set I = NNN
	void instruction_ANNN(const s32 NNN) {
		mRegisterI = static_cast<u16>(NNN);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region B instruction branch
/*==================================================================*/

	// BXNN - jump to NNN + V0
	void instruction_BNNN(const s32 NNN) {
		jumpProgramTo(NNN + mRegisterV[0]);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region C instruction branch
/*==================================================================*/

	// CXNN - set VX = rnd(256) & NN
	void instruction_CxNN(const s32 X, const s32 NN) {
		mRegisterV[X] = static_cast<u8>(Wrand.get() & NN);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region D instruction branch
/*==================================================================*/

	void drawByte(
		s32 X, s32 Y,
		const usz DATA
	) {
		switch (DATA) {
			case 0b00000000:
				return;
			case 0b10000000:
				if (Quirk.wrapSprite) { X &= mDisplayWb; }
				if (X < mDisplayW) {
					if (!(mDisplayBuffer[Y * mDisplayW + X] ^= 1))
						{ mRegisterV[0xF] = 1; }
				}
				return;
			default:
				if (Quirk.wrapSprite) { X &= 0x3F; }
				else if (X >= mDisplayW) { return; }

				for (auto B{ 0 }; B++ < 8; ++X &= mDisplayWb) {
					if (DATA & 0x80 >> B) {
						if (!(mDisplayBuffer[Y * mDisplayW + X] ^= 1))
							{ mRegisterV[0xF] = 1; }
					}
					if (!Quirk.wrapSprite && X == mDisplayWb) { return; }
				}
				return;
		}
	}

	void drawSprite(
		s32 X,
		s32 Y,
		s32 N
	) {
		X = mRegisterV[X] & mDisplayWb;
		Y = mRegisterV[Y] & mDisplayHb;

		mRegisterV[0xF] = 0;

		switch (N) {
			case 1:
				drawByte(X, Y, readMemoryI());
				break;
			case 0:
				for (auto H{ 0 }, I{ 0 }; H < 16; ++H, ++Y &= mDisplayHb)
				{
					drawByte(X + 0, Y, readMemoryI(I++));
					drawByte(X + 8, Y, readMemoryI(I++));
					if (!Quirk.wrapSprite && Y == mDisplayHb) { break; }
				}
				break;
			default:
				for (auto H{ 0 }; H < N; ++H, ++Y &= mDisplayHb)
				{
					drawByte(X, Y, readMemoryI(H));
					if (!Quirk.wrapSprite && Y == mDisplayHb) { break; }
				}
				break;
		}
	}

	// DXYN - draw N sprite rows at VX and VY
	void instruction_DxyN(const s32 X, const s32 Y, const s32 N) {
		if (Quirk.waitVblank) [[unlikely]]
			{ setInterrupt(Interrupt::FRAME); }
		drawSprite(X, Y, N);
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region E instruction branch
/*==================================================================*/

	// EX9E - skip next instruction if key VX down (p1)
	void instruction_Ex9E(const s32 X) {
		if ( Input.keyHeld_P1(mRegisterV[X])) { mProgCounter += 2; }
	}
	// EXA1 - skip next instruction if key VX up (p1)
	void instruction_ExA1(const s32 X) {
		if (!Input.keyHeld_P1(mRegisterV[X])) { mProgCounter += 2; }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region F instruction branch
/*==================================================================*/

	// FX07 - set VX = delay timer
	void instruction_Fx07(const s32 X) {
		mRegisterV[X] = mDelayTimer;
	}
	// FX0A - set VX = key, wait for keypress
	void instruction_Fx0A(const s32 X) {
		setInterrupt(Interrupt::INPUT);
		mInputReg = static_cast<u8>(X);
	}
	// FX15 - set delay timer = VX
	void instruction_Fx15(const s32 X) {
		mDelayTimer = mRegisterV[X];
	}
	// FX18 - set sound timer = VX
	void instruction_Fx18(const s32 X) {
		mAudioTone  = calcAudioTone();
		mSoundTimer = mRegisterV[X] + (mRegisterV[X] == 1);
	}
	// FX1E - set I = I + VX
	void instruction_Fx1E(const s32 X) {
		mRegisterI += mRegisterV[X];
	}
	// FX29 - point I to 5 byte hex sprite from value in VX
	void instruction_Fx29(const s32 X) {
		mRegisterI = (mRegisterV[X] & 0xF) * 5;
	}
	// FX33 - store BCD of VX to RAM at I, I+1, I+2
	void instruction_Fx33(const s32 X) {
		writeMemoryI(mRegisterV[X] / 100,     0);
		writeMemoryI(mRegisterV[X] / 10 % 10, 1);
		writeMemoryI(mRegisterV[X]      % 10, 2);
	}
	// FX55 - store V0..VX to RAM at I..I+X
	void instruction_Fx55(const s32 X) {
		for (auto idx{ 0 }; idx <= X; ++idx)
			{ writeMemoryI(mRegisterV[idx], idx); }
		if (!Quirk.idxRegNoInc) [[likely]]
			{ mRegisterI += static_cast<u16>(X + 1); }
	}
	// FX65 - load V0..VX from RAM at I..I+X
	void instruction_Fx65(const s32 X) {
		for (auto idx{ 0 }; idx <= X; ++idx)
			{ mRegisterV[idx] = readMemoryI(idx); }
		if (!Quirk.idxRegNoInc) [[likely]]
			{ mRegisterI += static_cast<u16>(X + 1); }
	}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

};
