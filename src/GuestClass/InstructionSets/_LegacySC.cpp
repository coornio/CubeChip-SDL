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

FunctionsForLegacySC::FunctionsForLegacySC(VM_Guest* parent)
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
	if (std::cmp_equal(byte, 0)) return 0;
	byte = (byte << 4u | byte) & 0x0F0Fu;
	byte = (byte << 2u | byte) & 0x3333u;
	byte = (byte << 1u | byte) & 0x5555u;
	return  byte << 1u | byte;
}

void FunctionsForLegacySC::drawByte(
	std::int32_t X, std::int32_t Y,
	const std::size_t DATA
) {
	if (!DATA || std::cmp_equal(X, vm->Plane.W)) return;

	for (std::size_t B{ 0 }; std::cmp_less(B, 8); ++B) {
		if (DATA >> (7u - B) & 0x1u) {
			auto& elem{ vm->Mem->displayBuffer[0].at_raw(Y, X) };
			if (std::cmp_not_equal(elem, 0)) {
				++vm->Reg->V[0xF];
			}
			elem ^= 1;
		}
		if (std::cmp_equal(++X, vm->Plane.W)) {
			if (vm->Quirk.wrapSprite)
				X &= vm->Plane.Wb;
			else return;
		}
	}
}

void FunctionsForLegacySC::drawShort(
	std::int32_t X, std::int32_t Y,
	const std::size_t DATA
) {
	if (!DATA) return;

	for (std::size_t B{ 0 }; std::cmp_less(B, 16); ++B) {
		auto& elem0{ vm->Mem->displayBuffer[0].at_raw(Y + 0, X) };
		auto& elem1{ vm->Mem->displayBuffer[0].at_raw(Y + 1, X) };
		if (DATA >> (15u - B) & 0x1u) {
			if (std::cmp_not_equal(elem0, 0)) {
				vm->Reg->V[0xF] = 1;
			}
			elem1 = elem0 ^= 1;
		} else {
			elem1 = elem0;
		}
		if (std::cmp_equal(++X, vm->Plane.W)) {
			if (vm->Quirk.wrapSprite)
				X &= vm->Plane.Wb;
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
	printf("\nDXYN called, %d rows", N);

	for (auto Y{ 0 }; std::cmp_less(Y, N); ++Y) {
		if (mode == vm->Resolution::LO) { // lores 8xN (doubled)
			drawShort(VX, VY, bitBloat(vm->mrw(I++)));
		} else {                          // hires 8xN / 16xN
			if (true) drawByte(VX + 0, VY, vm->mrw(I++));
			if (wide) drawByte(VX + 8, VY, vm->mrw(I++));
		}

		if (std::cmp_greater(VY += mode, vm->Plane.Hb)) {
			if (vm->Quirk.wrapSprite)
				VY &= vm->Plane.Hb;
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
	auto mode{ vm->Program->screenMode };
	auto Xb{ vm->Plane.W >> 3 };

	if (N) {
		if (mode == vm->Resolution::LO) {
			VY <<= 1; VX <<= 1; N <<= 1;
		}

		const auto X{ VX >> 3 };
		for (auto _Y{ 0 }; std::cmp_less(_Y, N); ++_Y) {
			const auto Y{ VY + _Y & vm->Plane.Hb };
			vm->Mem->color8xBuffer[Y][X + 0] = vm->Color->getFore8X(idx);
			if (mode != vm->Resolution::LO) continue;
			vm->Mem->color8xBuffer[Y][X + 1] = vm->Color->getFore8X(idx);
		}
		vm->State.chip8X_hires = true;
	}
	else {
		VY &= 0x77, VX &= 0x77;

		if (mode == vm->Resolution::LO)
			VY <<= 1, VX <<= 1;

		const auto H{ (VY >> 4) + mode };
		const auto W{ (VX >> 4) + mode };

		for (auto _Y{ 0 }; std::cmp_less(_Y, H); ++_Y) {
			const auto Y{ ((VY + _Y) << 2) & vm->Plane.Hb };
			for (auto _X{ 0 }; std::cmp_less(_X, W); ++_X) {
				const auto X{ VX + _X & Xb };
				vm->Mem->color8xBuffer[Y + 0][X] = vm->Color->getFore8X(idx);
				//if (mode != vm->Resolution::LO) continue;
				//vm->Mem->color8xBuffer[Y + 1][X] = vm->Color->getFore8X(idx);
			}
		}
		vm->State.chip8X_hires = false;
	}
}
