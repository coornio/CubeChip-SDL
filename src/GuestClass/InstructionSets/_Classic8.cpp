/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <utility>

#include "Interface.hpp"
#include "../Guest.hpp"
#include "../Registers.hpp"
#include "../MemoryBanks.hpp"
#include "../DisplayColors.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForClassic8                  */
/*------------------------------------------------------------------*/

FunctionsForClassic8::FunctionsForClassic8(VM_Guest* parent)
	: vm{ parent }
{}

void FunctionsForClassic8::scrollUP(const std::int32_t N) {
	vm->Mem->displayBuffer[0].shift(-N, 0);
}
void FunctionsForClassic8::scrollDN(const std::int32_t N) {
	vm->Mem->displayBuffer[0].shift(+N, 0);
}
void FunctionsForClassic8::scrollLT(const std::int32_t) {
	vm->Mem->displayBuffer[0].shift(0, -4);
}
void FunctionsForClassic8::scrollRT(const std::int32_t) {
	vm->Mem->displayBuffer[0].shift(0, +4);
}

/*------------------------------------------------------------------*/

void FunctionsForClassic8::drawByte(
	std::int32_t X, std::int32_t Y,
	const std::size_t DATA
) {
	if (!DATA || std::cmp_equal(X, vm->Plane.W)) return;

	for (std::size_t B{ 0 }; std::cmp_less(B, 8); ++B) {
		if (DATA >> (7u - B) & 0x1u) {
			auto& elem{ vm->Mem->displayBuffer[0].at_raw(Y, X) };
			if (std::cmp_not_equal(elem, 0)) {
				vm->Reg->V[0xF] = 1;
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

void FunctionsForClassic8::drawSprite(
	std::int32_t VX, std::int32_t VY,
	std::int32_t  N, std::uint32_t I
) {
	VX &= vm->Plane.Wb;
	VY &= vm->Plane.Hb;
	vm->Reg->V[0xF] = 0;

	const bool wide{ N == 0 };
	if (wide) N = 16;

	for (auto Y{ 0 }; std::cmp_less(Y, N); ++Y) {
		if (true) drawByte(VX + 0, VY, vm->mrw(I++));
		if (wide) drawByte(VX + 8, VY, vm->mrw(I++));
		
		if (std::cmp_greater(++VY, vm->Plane.Hb)) {
			if (vm->Quirk.wrapSprite)
				VY &= vm->Plane.Hb;
			else return;
		}
	}
}

void FunctionsForClassic8::drawColors(
	const std::int32_t VX,
	const std::int32_t VY,
	const std::int32_t idx,
	const std::int32_t N
) {
	const auto Xb{ (vm->Plane.W >> 3) - 1 };
	if (N) {
		const auto X{ VX >> 3 };
		for (auto _Y{ 0 }; std::cmp_less(_Y, N); ++_Y) {
			const auto Y{ VY + _Y & vm->Plane.Hb };
			vm->Mem->color8xBuffer.at_raw(Y, X) = vm->Color->getFore8X(idx);
		}
		vm->State.chip8X_hires = true;
	}
	else {
		const auto H{ (VY >> 4) + 1 };
		const auto W{ (VX >> 4) + 1 };

		for (auto _Y{ 0 }; std::cmp_less(_Y, H); ++_Y) {
			const auto Y{ ((VY + _Y) << 2) & vm->Plane.Hb };
			for (auto _X{ 0 }; std::cmp_less(_X, W); ++_X) {
				const auto X{ VX + _X & Xb };
				vm->Mem->color8xBuffer.at_raw(Y, X) = vm->Color->getFore8X(idx);
			}
		}
		vm->State.chip8X_hires = false;
	}
}
