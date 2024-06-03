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
	const std::size_t DATA, bool& HIT
) {
	if (!DATA || X == vm->Plane.W) return;

	for (auto B{ 0 }; B < 8; ++B) {
		if (DATA >> (7 - B) & 0x1) {
			auto& elem{ vm->Mem->displayBuffer[0].at_raw(Y, X) };
			if (elem) { HIT = true; }
			elem ^= 1;
		}
		if (++X == vm->Plane.W) {
			if (vm->Quirk.wrapSprite) { X &= vm->Plane.Wb; }
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
			if (elem0) { vm->Reg->V[0xF] = 1; }
			elem1 = elem0 ^= 1;
		} else {
			elem1 = elem0;
		}
		if (++X == vm->Plane.W) {
			if (vm->Quirk.wrapSprite) { X &= vm->Plane.Wb; }
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
	const auto lores{ vm->Program->screenLores };

	VX *= 1 + lores; VX &= vm->Plane.Wb;
	VY *= 1 + lores; VY &= vm->Plane.Hb;
	vm->Reg->V[0xF] = 0;

	const bool wide{ N == 0 };
	if (wide) { N = 16; }

	for (auto Y{ 0 }; Y < N; ++Y) {
		if (vm->Program->screenHires) { // hires 8xN / 16xN
			bool ltHit{}, rtHit{};
			if (true) { drawByte(VX + 0, VY, vm->mrw(I++), ltHit); }
			if (wide) { drawByte(VX + 8, VY, vm->mrw(I++), rtHit); }
			vm->Reg->V[0xF] += ltHit || rtHit;
		}
		else {                        // lores 8xN (doubled)
			drawShort(VX, VY, bitBloat(vm->mrw(I++)));
		}

		if ((VY += 1 + lores) == vm->Plane.H) {
			if (vm->Quirk.wrapSprite) { VY &= vm->Plane.Hb; }
			else return;
		}
	}
}

void FunctionsForLegacySC::drawLoresColor(
	std::int32_t VX,
	std::int32_t VY,
	std::int32_t idx
) {
	const auto lores{ vm->Program->screenLores };
	const auto color{ vm->Color->getFore8X(idx) };

	VY &= 0x77; VX &= 0x77;

	if (lores) { VX <<= 1; }

	for (auto Y{ 0 }, H{ (VY >> 4) + 1 }; Y < H; ++Y) {
		for (auto X{ 0 }, W{ (VX >> 4) + 1 + lores }; X < W; ++X) {
			vm->Mem->color8xBuffer.at_wrap(((VY + Y) << (2 + lores)) + 0, VX + X) = color;
			if (!lores) continue;
			vm->Mem->color8xBuffer.at_wrap(((VY + Y) << (2 + lores)) + 1, VX + X) = color;
		}
	}
}

void FunctionsForLegacySC::drawHiresColor(
	std::int32_t VX,
	std::int32_t VY,
	std::int32_t idx,
	std::int32_t N
) {
	const auto lores{ vm->Program->screenLores };
	const auto color{ vm->Color->getFore8X(idx) };

	if (lores) {
		VY <<= 1;
		VX <<= 1;
		N  <<= 1;
	}

	for (auto Y{ 0 }, X{ VX >> 3 }; Y < N; ++Y) {
		vm->Mem->color8xBuffer.at_wrap(VY + Y, X + 0) = color;
		if (!lores) continue;
		vm->Mem->color8xBuffer.at_wrap(VY + Y, X + 1) = color;
	}
}
