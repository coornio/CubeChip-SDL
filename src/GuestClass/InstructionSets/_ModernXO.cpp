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
	if (!vm->Plane.selected) { return; }

	for (auto P{ 0 }; P < 4; ++P) {
		if (vm->Plane.selected & (1 << P)) {
			vm->Mem->displayBuffer[P].shift(-N, 0);
		}
	}
}
void FunctionsForModernXO::scrollDN(const std::int32_t N) {
	if (!vm->Plane.selected) { return; }

	for (auto P{ 0 }; P < 4; ++P) {
		if (vm->Plane.selected & (1 << P)) {
			vm->Mem->displayBuffer[P].shift(+N, 0);
		}
	}
}
void FunctionsForModernXO::scrollLT(const std::int32_t) {
	if (!vm->Plane.selected) { return; }

	for (auto P{ 0 }; P < 4; ++P) {
		if (vm->Plane.selected & (1 << P)) {
			vm->Mem->displayBuffer[P].shift(0, -4);
		}
	}
}
void FunctionsForModernXO::scrollRT(const std::int32_t) {
	if (!vm->Plane.selected) { return; }

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
	if (!DATA || X >= vm->Plane.W) { return; }

	for (auto B{ 0 }; B++ < 8; ++X &= vm->Plane.Wb)
	{
		if (DATA >> (8 - B) & 0x1) {
			auto& pixel{ vm->Mem->displayBuffer[P].at_raw(Y, X) };
			if (pixel) { vm->Reg->V[0xF] = 1; }

			switch (vm->Plane.brush) {
				case BrushType::XOR: pixel ^=  1; break;
				case BrushType::SUB: pixel &= ~1; break;
				case BrushType::ADD: pixel |=  1; break;
			}
		}
		if (!vm->Quirk.wrapSprite && X == vm->Plane.Wb) { return; }
	}
}

void FunctionsForModernXO::drawSprite(
	std::int32_t VX, std::int32_t  VY,
	std::int32_t  N, std::uint32_t IR
) {
	vm->Reg->V[0xF] = 0;
	if (!vm->Plane.selected) { return; }

	VX &= vm->Plane.Wb;
	VY &= vm->Plane.Hb;

	const bool wide{ N == 0 };
	if (wide) { N = 16; }

	for (auto P{ 0 }; P < 4; ++P)
	{
		if (!(vm->Plane.selected & (1 << P))) { continue; }

		for (auto H{ 0 }, Y{ VY }; H < N; ++H, ++Y &= vm->Plane.Hb)
		{
			if (true) { drawByte(VX + 0, Y, P, vm->mrw(IR++)); }
			if (wide) { drawByte(VX + 8, Y, P, vm->mrw(IR++)); }
			if (!vm->Quirk.wrapSprite && Y == vm->Plane.Hb) { break; }
		}
	}
}
