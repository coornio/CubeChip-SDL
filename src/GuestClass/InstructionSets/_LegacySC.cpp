/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Interface.hpp"
#include "../Guest.hpp"
#include "../Registers.hpp"
#include "../ProgramControl.hpp"
#include "../MemoryBanks.hpp"
#include "../DisplayColors.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForLegacySC                  */
/*------------------------------------------------------------------*/

FunctionsForLegacySC::FunctionsForLegacySC(VM_Guest* parent) noexcept
	: vm{ parent }
{}

void FunctionsForLegacySC::scrollUP(const std::int32_t N) {
	vm->Mem->displayBuffer[0].shift(-N, 0);
}
void FunctionsForLegacySC::scrollDN(const std::int32_t N) {
	vm->Mem->displayBuffer[0].shift(+N, 0);
}
void FunctionsForLegacySC::scrollLT(const std::int32_t) {
	vm->Mem->displayBuffer[0].shift(0, -4);
}
void FunctionsForLegacySC::scrollRT(const std::int32_t) {
	vm->Mem->displayBuffer[0].shift(0, +4);
}

/*------------------------------------------------------------------*/

std::size_t FunctionsForLegacySC::bitBloat(std::size_t byte) {
	if (!byte) return 0;
	byte = (byte << 4 | byte) & 0x0F0F;
	byte = (byte << 2 | byte) & 0x3333;
	byte = (byte << 1 | byte) & 0x5555;
	return  byte << 1 | byte;
}

void FunctionsForLegacySC::drawByte(
	std::int32_t X, std::int32_t Y,
	const std::size_t DATA
) {
	if (!DATA || X == vm->Plane.W) return;

	for (auto B{ 0 }; B < 8; ++B) {
		if (DATA >> (7 - B) & 0x1) {
			auto& elem{ vm->Mem->displayBuffer[0].at_raw(Y, X) };
			if (elem) ++vm->Reg->V[0xF];
			elem ^= 1;
		}
		if (++X == vm->Plane.W) {
			if (vm->Quirk.wrapSprite) X &= vm->Plane.Wb;
			else return;
		}
	}
}

void FunctionsForLegacySC::drawShort(
	std::int32_t X, std::int32_t Y,
	const std::size_t DATA
) {
	if (!DATA) return;

	for (auto B{ 0 }; B < 16; ++B) {
		auto& elem0{ vm->Mem->displayBuffer[0].at_raw(Y + 0, X) };
		auto& elem1{ vm->Mem->displayBuffer[0].at_raw(Y + 1, X) };
		if (DATA >> (15 - B) & 0x1) {
			if (elem0) vm->Reg->V[0xF] = 1;
			elem1 = elem0 ^= 1;
		} else {
			elem1 = elem0;
		}
		if (++X == vm->Plane.W) {
			if (vm->Quirk.wrapSprite) X &= vm->Plane.Wb;
			else return;
		}
	}
}

void FunctionsForLegacySC::drawSprite(
	std::int32_t VX,
	std::int32_t VY,
	std::int32_t  N,
	std::uint32_t I
) {
	const auto mode{ vm->Program->screenMode };

	VX *= mode; VX &= vm->Plane.Wb;
	VY *= mode; VY &= vm->Plane.Hb;
	vm->Reg->V[0xF] = 0;

	const bool wide{ N == 0 };
	if (wide) N = 16;

	for (auto Y{ 0 }; Y < N; ++Y) {
		if (mode == vm->Resolution::LO) { // lores 8xN (doubled)
			drawShort(VX, VY, bitBloat(vm->mrw(I++)));
		} else {                          // hires 8xN / 16xN
			if (true) drawByte(VX + 0, VY, vm->mrw(I++));
			if (wide) drawByte(VX + 8, VY, vm->mrw(I++));
		}

		if ((VY += mode) == vm->Plane.H) {
			if (vm->Quirk.wrapSprite) VY &= vm->Plane.Hb;
			else return;
		}
	}
}

void FunctionsForLegacySC::drawColors(
	std::int32_t VX,
	std::int32_t VY,
	std::int32_t idx,
	std::int32_t N
) {
	const auto mode{ vm->Program->screenMode };
	const auto color{ vm->Color->getFore8X(idx) };

	if (N) {
		if (mode == vm->Resolution::LO) {
			VY <<= 1; VX <<= 1; N <<= 1;
		}

		const auto X{ VX >> 3 };
		for (auto Y{ 0 }; Y < N; ++Y) {
			vm->Mem->color8xBuffer.at_wrap(VY + Y, X + 0) = color;
			if (mode != vm->Resolution::LO) continue;
			vm->Mem->color8xBuffer.at_wrap(VY + Y, X + 1) = color;
		}
		vm->State.chip8X_hires = true;
	}
	else {
		VY &= 0x77; VX &= 0x77;

		if (mode == vm->Resolution::LO) {
			VY <<= 1; VX <<= 1;
		}

		const auto H{ (VY >> 4) + mode };
		const auto W{ (VX >> 4) + mode };

		for (auto Y{ 0 }; Y < H; ++Y) {
			const auto _Y{ (VY + Y) << 2 };
			for (auto X{ 0 }; X < W; ++X) {
				const auto _X{ VX + X };
				vm->Mem->color8xBuffer.at_wrap(_Y + 0, _X) = color;
				//if (mode != vm->Resolution::LO) continue;
				//vm->Mem->color8xBuffer.at_wrap(_Y + 1, _X) = color;
			}
		}
		vm->State.chip8X_hires = false;
	}
}
