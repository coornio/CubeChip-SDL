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
	vm->isDisplayReady(true);
	vm->Mem->display.shift(-N, 0);
}
void FunctionsForClassic8::scrollDN(const std::int32_t N) {
	vm->isDisplayReady(true);
	vm->Mem->display.shift(+N, 0);
}
void FunctionsForClassic8::scrollLT(const std::int32_t) {
	vm->isDisplayReady(true);
	vm->Mem->display.shiftBit(0, -4, 8);
}
void FunctionsForClassic8::scrollRT(const std::int32_t) {
	vm->isDisplayReady(true);
	vm->Mem->display.shiftBit(0, +4, 8);
}

/*------------------------------------------------------------------*/

void FunctionsForClassic8::drawByte(
	const std::size_t L, const std::size_t SHL,
	const std::size_t R, const std::size_t SHR,
	const std::size_t Y, const std::size_t DATA
) {
	if (!DATA || std::cmp_greater_equal(L, vm->Plane.X)) return;
	const auto DATA_L{ DATA >> SHR & 0xFF };
	
	if (!vm->Reg->V[0xF]) [[unlikely]]
		vm->Reg->V[0xF]     = (vm->Mem->display[Y][L] & DATA_L) != 0;
	vm->Mem->display[Y][L] ^= DATA_L;

	if (!SHR || std::cmp_greater_equal(R, vm->Plane.X)) return;
	const auto DATA_R{ DATA << SHL & 0xFF };

	if (!vm->Reg->V[0xF]) [[unlikely]]
		vm->Reg->V[0xF]     = (vm->Mem->display[Y][R] & DATA_R) != 0;
	vm->Mem->display[Y][R] ^= DATA_R;
}

void FunctionsForClassic8::drawSprite(
	std::int32_t VX,
	std::int32_t VY,
	std::int32_t  N,
	std::uint32_t I
) {
	vm->isDisplayReady(true);

	VX &= vm->Plane.Wb;
	VY &= vm->Plane.Hb;
	vm->Reg->V[0xF] = 0;

	const bool wide{ N == 0 };
	N = VY + (wide ? 16 : N);

	const auto SHR{ VX & 7 };
	const auto SHL{ 8 - SHR };

	auto X0{ VX >> 3 };
	auto X1{ X0 + 1 };
	auto X2{ X0 + 2 };

	if (vm->Quirk.wrapSprite) {
		X1 &= vm->Plane.Xb;
		X2 &= vm->Plane.Xb;
	}

	for (auto H{ VY }; std::cmp_less(H, N); ++H) {
		if (!vm->Quirk.wrapSprite)
			if (std::cmp_greater_equal(H, vm->Plane.H)) break;

		const auto Y{ H & vm->Plane.Hb };

		drawByte(X0, SHL, X1, SHR, Y, vm->mrw(I++));
		if (!wide) continue;
		drawByte(X1, SHL, X2, SHR, Y, vm->mrw(I++));
	}
}

void FunctionsForClassic8::drawColors(
	const std::int32_t VX,
	const std::int32_t VY,
	const std::int32_t idx,
	const std::int32_t N
) {
	vm->isDisplayReady(true);

	if (N) {
		const auto X{ VX >> 3 };
		for (auto _Y{ 0 }; std::cmp_less(_Y, N); ++_Y) {
			const auto Y{ VY + _Y & vm->Plane.Hb };
			vm->Mem->bufColor8x[Y][X] = vm->Color->getFore8X(idx);
		}
		vm->State.chip8X_hires = true;
	}
	else {
		const auto H{ (VY >> 4) + 1 };
		const auto W{ (VX >> 4) + 1 };

		for (auto _Y{ 0 }; std::cmp_less(_Y, H); ++_Y) {
			const auto Y{ ((VY + _Y) << 2) & vm->Plane.Hb };
			for (auto _X{ 0 }; std::cmp_less(_X, W); ++_X) {
				const auto X{ VX + _X & vm->Plane.Xb };
				vm->Mem->bufColor8x[Y][X] = vm->Color->getFore8X(idx);
			}
		}
		vm->State.chip8X_hires = false;
	}
}
