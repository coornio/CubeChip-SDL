/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../../../Assistants/Map2D.hpp"

#include "../Chip8_CoreInterface.hpp"

/*==================================================================*/

class XOCHIP final : public Chip8_CoreInterface {
	static constexpr u32 cTotalMemory{ 65536 };
	static constexpr u32 cSafezoneOOB{    32 };
	static constexpr u32 cGameLoadPos{   512 };
	static constexpr u32 cStartOffset{   512 };
	static constexpr f32 cRefreshRate{ 60.0f };
	static constexpr s32 cScreenSizeX{    64 };
	static constexpr s32 cScreenSizeY{    32 };
	static constexpr s32 cInstSpeedHi{ 50000 };
	static constexpr s32 cInstSpeedLo{  1000 };

private:
	std::array<u32, 16> mBitsColor{};

	void setColorBit332(const s32 bit, const s32 color) noexcept;

/*==================================================================*/

	std::array<u8, 16> mPatternBuf{};
	s32 mAudioPitch{ 64 };

/*==================================================================*/

	Map2D<u8> mDisplayBuffer[4];

	std::array<u8, cTotalMemory + cSafezoneOOB>
		mMemoryBank{};

	void writeMemoryI(const u32 value, const u32 pos) noexcept {
		const auto index{ mRegisterI + pos };
		if (!(index & cTotalMemory)) [[likely]]
			{ mMemoryBank[index] = value & 0xFF; }
	}

	auto readMemoryI(const u32 pos) const noexcept {
		return mMemoryBank[mRegisterI + pos];
	}

/*==================================================================*/

	auto  NNNN() const noexcept { return mMemoryBank[mCurrentPC] << 8 | mMemoryBank[mCurrentPC + 1]; }

public:
	XOCHIP();

	static constexpr bool testGameSize(const usz size) noexcept {
		return size + cGameLoadPos <= cTotalMemory;
	}

private:
	void instructionLoop() noexcept override;

	void renderAudioData() override;
	void renderVideoData() override;

	void prepDisplayArea(const Resolution mode) override;

	void skipInstruction() noexcept override;

	void scrollDisplayUP(const s32 N);
	void scrollDisplayDN(const s32 N);
	void scrollDisplayLT();
	void scrollDisplayRT();

/*==================================================================*/
	#pragma region 0 instruction branch

	// 00DN - scroll selected color plane N lines down
	void instruction_00CN(const s32 N) noexcept;
	// 00DN - scroll selected color plane N lines up
	void instruction_00DN(const s32 N) noexcept;
	// 00E0 - erase selected color plane
	void instruction_00E0() noexcept;
	// 00EE - return from subroutine
	void instruction_00EE() noexcept;
	// 00FB - scroll selected color plane 4 pixels right
	void instruction_00FB() noexcept;
	// 00FC - scroll selected color plane 4 pixels left
	void instruction_00FC() noexcept;
	// 00FD - stop signal
	void instruction_00FD() noexcept;
	// 00FE - display res == 64x32, erase whole display
	void instruction_00FE() noexcept;
	// 00FF - display res == 128x64, erase whole display
	void instruction_00FF() noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 1 instruction branch

	// 1NNN - jump to NNN
	void instruction_1NNN(const s32 NNN) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 2 instruction branch

	// 2NNN - call subroutine at NNN
	void instruction_2NNN(const s32 NNN) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 3 instruction branch

	// 3XNN - skip next instruction if VX == NN
	void instruction_3xNN(const s32 X, const s32 NN) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 4 instruction branch

	// 4XNN - skip next instruction if VX != NN
	void instruction_4xNN(const s32 X, const s32 NN) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 5 instruction branch

	// 5XY0 - skip next instruction if VX == VY
	void instruction_5xy0(const s32 X, const s32 Y) noexcept;
	// 5XY2 - store range of registers to memory
	void instruction_5xy2(const s32 X, const s32 Y) noexcept;
	// 5XY3 - load range of registers from memory
	void instruction_5xy3(const s32 X, const s32 Y) noexcept;
	// 5XY4 - load range of colors from memory *EXPERIMENTAL*
	void instruction_5xy4(const s32 X, const s32 Y) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 6 instruction branch

	// 6XNN - set VX = NN
	void instruction_6xNN(const s32 X, const s32 NN) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 7 instruction branch

	// 7XNN - set VX = VX + NN
	void instruction_7xNN(const s32 X, const s32 NN) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 8 instruction branch

	// 8XY0 - set VX = VY
	void instruction_8xy0(const s32 X, const s32 Y) noexcept;
	// 8XY1 - set VX = VX | VY
	void instruction_8xy1(const s32 X, const s32 Y) noexcept;
	// 8XY2 - set VX = VX & VY
	void instruction_8xy2(const s32 X, const s32 Y) noexcept;
	// 8XY3 - set VX = VX ^ VY
	void instruction_8xy3(const s32 X, const s32 Y) noexcept;
	// 8XY4 - set VX = VX + VY, VF = carry
	void instruction_8xy4(const s32 X, const s32 Y) noexcept;
	// 8XY5 - set VX = VX - VY, VF = !borrow
	void instruction_8xy5(const s32 X, const s32 Y) noexcept;
	// 8XY7 - set VX = VY - VX, VF = !borrow
	void instruction_8xy7(const s32 X, const s32 Y) noexcept;
	// 8XY6 - set VX = VY >> 1, VF = carry
	void instruction_8xy6(const s32 X, const s32 Y) noexcept;
	// 8XYE - set VX = VY << 1, VF = carry
	void instruction_8xyE(const s32 X, const s32 Y) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region 9 instruction branch

	// 9XY0 - skip next instruction if VX != VY
	void instruction_9xy0(const s32 X, const s32 Y) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region A instruction branch

	// ANNN - set I = NNN
	void instruction_ANNN(const s32 NNN) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region B instruction branch

	// BNNN - jump to NNN + V0
	void instruction_BNNN(const s32 NNN) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region C instruction branch

	// CXNN - set VX = rnd(256) & NN
	void instruction_CxNN(const s32 X, const s32 NN) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region D instruction branch

	void drawByte(s32 X, s32 Y, const s32 P, const u32 DATA) noexcept;

	// DXYN - draw N sprite rows at VX and VY
	void instruction_DxyN(const s32 X, const s32 Y, const s32 N) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region E instruction branch

	// EX9E - skip next instruction if key VX down (p1)
	void instruction_Ex9E(const s32 X) noexcept;
	// EXA1 - skip next instruction if key VX up (p1)
	void instruction_ExA1(const s32 X) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region F instruction branch

	// F000 - set I = NEXT NNNN then skip instruction
	void instruction_F000() noexcept;
	// F002 - load 16-byte audio pattern from RAM at I
	void instruction_F002() noexcept;
	// FN01 - set plane drawing to N
	void instruction_FN01(const s32 N) noexcept;
	// FX07 - set VX = delay timer
	void instruction_Fx07(const s32 X) noexcept;
	// FX0A - set VX = key, wait for keypress
	void instruction_Fx0A(const s32 X) noexcept;
	// FX15 - set delay timer = VX
	void instruction_Fx15(const s32 X) noexcept;
	// FX18 - set sound timer = VX
	void instruction_Fx18(const s32 X) noexcept;
	// FX1E - set I = I + VX
	void instruction_Fx1E(const s32 X) noexcept;
	// FX29 - set I to 5-byte hex sprite from VX
	void instruction_Fx29(const s32 X) noexcept;
	// FX30 - set I to 10-byte hex sprite from VX
	void instruction_Fx30(const s32 X) noexcept;
	// FX33 - store BCD of VX to RAM at I..I+2
	void instruction_Fx33(const s32 X) noexcept;
	// FX3A - set sound pitch = VX
	void instruction_Fx3A(const s32 X) noexcept;
	// FN55 - store V0..VN to RAM at I..I+N
	void instruction_FN55(const s32 N) noexcept;
	// FN65 - load V0..VN from RAM at I..I+N
	void instruction_FN65(const s32 N) noexcept;
	// FN75 - store V0..VN to the permanent regs
	void instruction_FN75(const s32 N) noexcept;
	// FN85 - load V0..VN from the permanent regs
	void instruction_FN85(const s32 N) noexcept;

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
};
