/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Interface.hpp"
#include "../Guest.hpp"
#include "../Registers.hpp"
#include "../MemoryBanks.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForModernXO                  */
/*------------------------------------------------------------------*/

FunctionsForModernXO::FunctionsForModernXO(VM_Guest* parent) noexcept
	: vm{ parent }
{}

void FunctionsForModernXO::scrollUP(const std::int32_t N) {
	if (!vm->Plane.selected) return;

	for (auto P{ 0 }; P < 4; ++P) {
		if (vm->Plane.selected & (1 << P)) {
			vm->Mem->displayBuffer[P].shift(-N, 0);
		}
	}
}
void FunctionsForModernXO::scrollDN(const std::int32_t N) {
	if (!vm->Plane.selected) return;

	for (auto P{ 0 }; P < 4; ++P) {
		if (vm->Plane.selected & (1 << P)) {
			vm->Mem->displayBuffer[P].shift(+N, 0);
		}
	}
}
void FunctionsForModernXO::scrollLT(const std::int32_t) {
	if (!vm->Plane.selected) return;

	for (auto P{ 0 }; P < 4; ++P) {
		if (vm->Plane.selected & (1 << P)) {
			vm->Mem->displayBuffer[P].shift(0, -4);
		}
	}
}
void FunctionsForModernXO::scrollRT(const std::int32_t) {
	if (!vm->Plane.selected) return;

	for (auto P{ 0 }; P < 4; ++P) {
		if (vm->Plane.selected & (1 << P)) {
			vm->Mem->displayBuffer[P].shift(0, +4);
		}
	}
}

/*------------------------------------------------------------------*/

void FunctionsForModernXO::drawByte(
	std::int32_t X, std::int32_t Y,
	const std::int32_t P,
	const std::size_t DATA
) {
	if (!DATA || X == vm->Plane.W) return;

	for (auto B{ 0 }; B < 8; ++B) {
		if (DATA >> (7 - B) & 0x1) {
			auto& elem{ vm->Mem->displayBuffer[P].at_raw(Y, X) };
			if (elem) { vm->Reg->V[0xF] = 1; }

			switch (vm->Plane.brush) {
				case BrushType::XOR: elem ^=  1; break;
				case BrushType::SUB: elem &= ~1; break;
				case BrushType::ADD: elem |=  1; break;
			}
		}
		if (++X == vm->Plane.W) {
			if (vm->Quirk.wrapSprite) { X &= vm->Plane.Wb; }
			else return;
		}
	}
}

void FunctionsForModernXO::drawSprite(
	std::int32_t VX,
	std::int32_t VY,
	std::int32_t  N,
	std::uint32_t I
) {
	if (!vm->Plane.selected) {
		vm->Reg->V[0xF] = 0;
		return;
	}

	VX &= vm->Plane.Wb;
	VY &= vm->Plane.Hb;
	vm->Reg->V[0xF] = 0;

	const bool wide{ N == 0 };
	if (wide) { N = 16; }

	for (auto P{ 0 }; P <4; ++P) {
		if (!(vm->Plane.selected & (1 << P))) continue;

		for (auto Y{ 0 }, _VY{ VY }; Y < N; ++Y) {
			if (true) { drawByte(VX + 0, _VY, P, vm->mrw(I++)); }
			if (wide) { drawByte(VX + 8, _VY, P, vm->mrw(I++)); }

			if (++_VY == vm->Plane.H) {
				if (vm->Quirk.wrapSprite) { _VY &= vm->Plane.Hb; }
				else return;
			}
		}
	}
}
