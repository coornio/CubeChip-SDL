/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../Assistants/Well512.hpp"

#include "../../HostClass/BasicVideoSpec.hpp"

#include "../Guest.hpp"
#include "../HexInput.hpp"
#include "../SoundCores.hpp"
#include "../DisplayTraits.hpp"


// experimental work

enum Behaviors {
	defaults = 0x0,
	clearVF = 0x1,
	jmpRegX = 0x2,
	shiftVX = 0x4,
	idxRegNoInc = 0x8,
	idxRegMinus = 0x10,
	waitVblank = 0x20,
	waitScroll = 0x40,
	wrapSprite = 0x80,
	lschip_rom = 0x100,
	chip8E_rom = 0x200,
	wave64_rom = 0x400,
	megaC8_rom = 0x800,
	gigaC8_rom = 0x1000,
	chip8X_rom = 0x2000,
	xochip_rom = 0x4000,
	hires_2paged = 0x10000,
	hires_4paged = 0x20000,
};

enum Platforms {
	LEGACY_CHIP8 =
	clearVF | waitVblank,
	MODERN_CHIP8 =
	defaults,
	LEGACY_SCHIP =
	clearVF | shiftVX | idxRegNoInc | waitVblank | lschip_rom,
	MODERN_SCHIP =
	defaults,
	XOCHIP =
	wrapSprite | xochip_rom,
	HWCHIP =
	wrapSprite | xochip_rom | wave64_rom,
	MEGACHIP =
	clearVF | shiftVX | idxRegNoInc | megaC8_rom,
	GIGACHIP =
	wrapSprite | gigaC8_rom,
	CHIP8E =
	clearVF | waitVblank | chip8E_rom,
	CHIP8X =
	clearVF | waitVblank | chip8X_rom,
	CHIP8X_TPD =
	clearVF | chip8X_rom | hires_2paged,
	CHIP8X_FPD =
	clearVF | chip8X_rom | hires_4paged,
	SCHIP8X =
	chip8X_rom,
	SCHIP8X_TPD,
	SCHIP8X_FPD,
};

static constexpr std::size_t variation{};

template <std::size_t variant>
void VM_Guest::instructionDecoder() { /*
	for (auto inst{ 0 }; inst < Program->mCyclesPerFrame; ++inst) {
		auto HI = Mem->mrw(Program->counter++);
		auto LO = Mem->mrw(Program->counter++);
		Program->opcode = HI << 8 | LO;

		auto   X{ HI & 0xF };
		auto   Y{ LO >>  4 };
		auto   N{ LO & 0xF };
		auto NNN{ Program->opcode & 0xFFF };

		switch (HI >> 4) {
			case 0x0: switch (NNN) {
				case 0x0E0:								// 00E0 - erase whole display
					Mem->modifyViewport(BrushType::CLR);
					break;
				case 0x0EE:								// 00EE - return from subroutine
					Reg->routineReturn();
					break;
				[[unlikely]] default: Program->requestHalt();
			} break;
			case 0x1:									// 1NNN - jump to NNN; stop if PC == NNN (inf loop)
				Program->jumpInstruction(NNN);
				break;
			case 0x2:									// 2NNN - call subroutine
				Reg->routineCall(NNN);
				break;
			case 0x3:									// 3XNN - skip next instruction if VX == NN
				if (Reg->V[X] == LO) Program->skipInstruction();
				break;
			case 0x4:									// 4XNN - skip next instruction if VX != NN
				if (Reg->V[X] != LO) Program->skipInstruction();
				break;
			case 0x5:									// 5XY0 - skip next instruction if VX == VY
				if (N) [[unlikely]] Program->requestHalt();
				if (Reg->V[X] == Reg->V[Y]) Program->skipInstruction();
				break;
			case 0x6:									// 6XNN - set VX = NN
				Reg->V[X] = LO;
				break;
			case 0x7:									// 7XNN - set VX = VX + NN
				Reg->V[X] += LO;
				break;
			case 0x8: switch (N) {
				case 0x0:								// 8XY0 - set VX = VY
					Reg->V[X] = Reg->V[Y];
					break;
				case 0x1:								// 8XY1 - set VX = VX | VY
					Reg->V[X] |= Reg->V[Y];
					if constexpr (variation & clearVF) Reg->V[0xF] = 0;
					break;
				case 0x2:								// 8XY2 - set VX = VX & VY
					Reg->V[X] &= Reg->V[Y];
					if constexpr (variation & clearVF) Reg->V[0xF] = 0;
					break;
				case 0x3:								// 8XY3 - set VX = VX ^ VY
					Reg->V[X] ^= Reg->V[Y];
					if constexpr (variation & clearVF) Reg->V[0xF] = 0;
					break;
				case 0x4: {								// 8XY4 - set VX = VX + VY, VF = carry
					const auto sum{ Reg->V[X] + Reg->V[Y] };
					Reg->V[X] = static_cast<uint8_t>(sum);
					Reg->V[0xF] = static_cast<uint8_t>(sum >> 8);
				} break;
				case 0x5: {								// 8XY5 - set VX = VX - VY, VF = !borrow
					const bool borrow{ Reg->V[X] < Reg->V[Y] };
					Reg->V[X] = Reg->V[X] - Reg->V[Y];
					Reg->V[0xF] = !borrow;
				} break;
				case 0x7: {								// 8XY7 - set VX = VY - VX, VF = !borrow
					const bool borrow{ Reg->V[Y] < Reg->V[X] };
					Reg->V[X] = Reg->V[Y] - Reg->V[X];
					Reg->V[0xF] = !borrow;
				};  break;
				case 0x6: {								// 8XY6 - set VX = VY >> 1, VF = carry
					if constexpr (variant & shiftVX) {
						const bool lsb{ (Reg->V[X] & 1) == 1 };
						Reg->V[X] = Reg->V[X] >> 1;
						Reg->V[0xF] = lsb;
					}
					else {
						const bool lsb{ (Reg->V[Y] & 1) == 1 };
						Reg->V[X] = Reg->V[Y] >> 1;
						Reg->V[0xF] = lsb;
					}
				} break;
				case 0xE: {								// 8XYE - set VX = VY << 1, VF = carry
					if constexpr (variant & shiftVX) {
						const bool msb{ (Reg->V[X] >> 7) == 1 };
						Reg->V[X] = Reg->V[X] << 1;
						Reg->V[0xF] = msb;
					}
					else {
						const bool msb{ (Reg->V[Y] >> 7) == 1 };
						Reg->V[X] = Reg->V[Y] << 1;
						Reg->V[0xF] = msb;
					}
				} break;
				[[unlikely]] default: Program->requestHalt();
			} break;
			case 0x9:									// 9XY0 - skip next instruction if VX != VY
				if (N) [[unlikely]] Program->requestHalt();;
				if (Reg->V[X] != Reg->V[Y]) Program->skipInstruction();
				break;
			case 0xA:									// ANNN - set I = NNN
				Reg->I = NNN;
				break;
			case 0xB:									// BXNN - jump to NNN + V0 (else VX *SCHIP*)
				Program->jumpInstruction(NNN + (Quirk.jmpRegX ? Reg->V[X] : Reg->V[0]));
				break;
			case 0xC:									// CXNN - set VX = rnd(256) & NN
				Reg->V[X] = Wrand->get() & LO;
				break;
			case 0xD:									// DXYN - draw N sprite rows at VX and VY
				if (Quirk.waitVblank) [[unlikely]] \
					Program->setInterrupt(Interrupt::ONCE);
				currFncSet->drawSprite(Reg->V[X], Reg->V[Y], N, Reg->I);
				break;
			case 0xE: switch (LO) {
				case 0x9E:								// EX9E - skip next instruction if key VX down (p1)
					if (Input->keyPressed(Reg->V[X], 0)) Program->skipInstruction();
					break;
				case 0xA1:								// EXA1 - skip next instruction if key VX up (p1)
					if (!Input->keyPressed(Reg->V[X], 0)) Program->skipInstruction();
					break;
				[[unlikely]] default: Program->requestHalt();
			} break;
			case 0xF: switch (LO) {
				case 0x07:								// FX07 - set VX = delay timer
					Reg->V[X] = Program->mDelayTimer;
					break;
				case 0x0A:								// FX0A - set VX = key, wait for keypress
					Sound->C8.setTone(Reg->SP, Program->counter);
					Program->setInterrupt(Interrupt::FX0A);
					break;
				case 0x15:								// FX15 - set delay timer = VX
					Program->mDelayTimer = Reg->V[X];
					break;
				case 0x18:								// FX18 - set sound timer = VX
					Sound->C8.setTone(Reg->SP, Program->counter);
					Sound->beepFx0A = false;
					Program->mSoundTimer = Reg->V[X] + (Reg->V[X] == 1);
					break;
				case 0x1E:								// FX1E - set I = I + VX
					Reg->I += Reg->V[X];
					break;
				case 0x29:								// FX29 - point I to 5 byte hex sprite from value in VX
					Reg->I = (Reg->V[X] & 0xF) * 5;
					break;
				case 0x33:								// FX33 - store BCD of VX to RAM at I, I+1, I+2
					Mem->mrw(Reg->I + 0) = Reg->V[X] / 100;
					Mem->mrw(Reg->I + 1) = Reg->V[X] / 10 % 10;
					Mem->mrw(Reg->I + 2) = Reg->V[X] % 10;
					break;
				case 0x55:								// FX55 - store V0..VX to RAM at I..I+X
					for (auto idx{ 0 }; idx <= X; ++idx)
						Mem->mrw(Reg->I + idx) = Reg->V[idx];
					if constexpr (!(variant & idxRegNoInc))
						Reg->I += X + !Quirk.idxRegMinus;
					break;
				case 0x65:								// FX65 - load V0..VX from RAM at I..I+X
					for (auto idx{ 0 }; idx <= X; ++idx)
						Reg->V[idx] = Mem->mrw(Reg->I + idx);
					if constexpr (!(variant & idxRegNoInc))
						Reg->I += X + !Quirk.idxRegMinus;
					break;
				[[unlikely]] default: Program->requestHalt();
			} break;
		}
	}*/
}
