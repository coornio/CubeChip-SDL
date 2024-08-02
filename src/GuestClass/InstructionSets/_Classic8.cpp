/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <utility>

#include "Interface.hpp"
#include "../Guest.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForClassic8                  */
/*------------------------------------------------------------------*/

FunctionsForClassic8::FunctionsForClassic8(VM_Guest& parent) noexcept
	: vm{ parent }
{}

/*------------------------------------------------------------------*/

void FunctionsForClassic8::scrollUP(const s32 N) {
	vm.displayBuffer[0].shift(-N, 0);
}
void FunctionsForClassic8::scrollDN(const s32 N) {
	vm.displayBuffer[0].shift(+N, 0);
}
void FunctionsForClassic8::scrollLT(const s32) {
	vm.displayBuffer[0].shift(0, -4);
}
void FunctionsForClassic8::scrollRT(const s32) {
	vm.displayBuffer[0].shift(0, +4);
}

/*------------------------------------------------------------------*/

void FunctionsForClassic8::drawByte(
	s32 X, s32 Y,
	const usz DATA
) {
	if (!DATA) { return; }
	if (vm.Quirk.wrapSprite) { X &= vm.Trait.Wb; }
	else if (X >= vm.Trait.W) { return; }

	for (auto B{ 0 }; B++ < 8; ++X &= vm.Trait.Wb) {
		if (DATA >> (8 - B) & 0x1) {
			auto& elem{ vm.displayBuffer[0].at_raw(Y, X) };
			if (elem) { vm.mRegisterV[0xF] = 1; }
			elem ^= 1;
		}
		if (!vm.Quirk.wrapSprite && X == vm.Trait.Wb) { return; }
	}
}

void FunctionsForClassic8::drawSprite(
	s32 X,
	s32 Y,
	s32 N
) {
	X = vm.mRegisterV[X] & vm.Trait.Wb;
	Y = vm.mRegisterV[Y] & vm.Trait.Hb;

	vm.mRegisterV[0xF] = 0;

	const bool wide{ N == 0 };
	if (wide) { N = 16; }

	for (auto H{ 0 }, I{ 0 }; H < N; ++H, ++Y &= vm.Trait.Hb)
	{
		if (true) { drawByte(X + 0, Y, vm.readMemoryI(I++)); }
		if (wide) { drawByte(X + 8, Y, vm.readMemoryI(I++)); }
		if (!vm.Quirk.wrapSprite && Y == vm.Trait.Hb) { break; }
	}
}

void FunctionsForClassic8::drawLoresColor(
	const s32 VX,
	const s32 VY,
	const s32 idx
) {
	for (auto Y{ 0 }, maxH{ VY >> 4 }; Y <= maxH; ++Y) {
		for (auto X{ 0 }, maxW{ VX >> 4 }; X <= maxW; ++X) {
			vm.color8xBuffer.at_wrap((VY + Y) << 2, VX + X) = 
				vm.getForegroundColor8X(idx);
		}
	}
	vm.Trait.mask8X = 0xFC;
}

void FunctionsForClassic8::drawHiresColor(
	const s32 VX,
	const s32 VY,
	const s32 idx,
	const s32 N
) {
	for (auto Y{ VY }, X{ VX >> 3 }; Y < VY + N; ++Y) {
		vm.color8xBuffer.at_wrap(Y, X) =
			vm.getForegroundColor8X(idx);
	}
	vm.Trait.mask8X = 0xFF;
}
