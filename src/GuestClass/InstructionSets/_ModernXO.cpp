/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Interface.hpp"
#include "../Guest.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForModernXO                  */
/*------------------------------------------------------------------*/

FunctionsForModernXO::FunctionsForModernXO(MEGACORE& parent) noexcept
	: vm{ parent }
{}

/*------------------------------------------------------------------*/

void FunctionsForModernXO::scrollUP(const s32 N) {
	if (!vm.Trait.maskPlane) { return; }

	for (auto P{ 0 }; P < 4; ++P) {
		if (vm.Trait.maskPlane & (1 << P)) {
			vm.displayBuffer[P].shift(-N, 0);
		}
	}
}
void FunctionsForModernXO::scrollDN(const s32 N) {
	if (!vm.Trait.maskPlane) { return; }

	for (auto P{ 0 }; P < 4; ++P) {
		if (vm.Trait.maskPlane & (1 << P)) {
			vm.displayBuffer[P].shift(+N, 0);
		}
	}
}
void FunctionsForModernXO::scrollLT(const s32) {
	if (!vm.Trait.maskPlane) { return; }

	for (auto P{ 0 }; P < 4; ++P) {
		if (vm.Trait.maskPlane & (1 << P)) {
			vm.displayBuffer[P].shift(0, -4);
		}
	}
}
void FunctionsForModernXO::scrollRT(const s32) {
	if (!vm.Trait.maskPlane) { return; }

	for (auto P{ 0 }; P < 4; ++P) {
		if (vm.Trait.maskPlane & (1 << P)) {
			vm.displayBuffer[P].shift(0, +4);
		}
	}
}

/*------------------------------------------------------------------*/

void FunctionsForModernXO::drawByte(
	s32 X, s32 Y,
	const s32 P,
	const usz  DATA
) {
	if (!DATA) { return; }
	if (vm.Quirk.wrapSprite) { X &= vm.Trait.Wb; }
	else if (X >= vm.Trait.W) { return; }

	for (auto B{ 0 }; B++ < 8; ++X &= vm.Trait.Wb)
	{
		if (DATA >> (8 - B) & 0x1) {
			auto& pixel{ vm.displayBuffer[P].at_raw(Y, X) };
			if (pixel) { vm.mRegisterV[0xF] = 1; }

			switch (vm.Trait.paintBrush) {
				case BrushType::XOR: pixel ^=  1; break;
				case BrushType::SUB: pixel &= ~1; break;
				case BrushType::ADD: pixel |=  1; break;
			}
		}
		if (!vm.Quirk.wrapSprite && X == vm.Trait.Wb) { return; }
	}
}

void FunctionsForModernXO::drawSprite(
	s32 _X, s32 _Y, s32 N
) {
	s32 VX{ vm.mRegisterV[_X] };
	s32 VY{ vm.mRegisterV[_Y] };

	vm.mRegisterV[0xF] = 0;
	if (!vm.Trait.maskPlane) { return; }

	VX &= vm.Trait.Wb;
	VY &= vm.Trait.Hb;

	const bool wide{ N == 0 };
	if (wide) { N = 16; }

	for (auto P{ 0 }, I{ 0 }; P < 4; ++P)
	{
		if (!(vm.Trait.maskPlane & (1 << P))) { continue; }

		for (auto H{ 0 }, Y{ VY }; H < N; ++H, ++Y &= vm.Trait.Hb)
		{
			if (true) { drawByte(VX + 0, Y, P, vm.readMemoryI(I++)); }
			if (wide) { drawByte(VX + 8, Y, P, vm.readMemoryI(I++)); }
			if (!vm.Quirk.wrapSprite && Y == vm.Trait.Hb) { break; }
		}
	}
}
