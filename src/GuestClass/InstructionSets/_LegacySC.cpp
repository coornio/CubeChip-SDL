/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Interface.hpp"
#include "../Guest.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForLegacySC                  */
/*------------------------------------------------------------------*/

FunctionsForLegacySC::FunctionsForLegacySC(MEGACORE& parent) noexcept
	: vm{ parent }
{}

/*------------------------------------------------------------------*/

void FunctionsForLegacySC::scrollUP(const s32 N) {
	vm.displayBuffer[0].shift(-N, 0);
}
void FunctionsForLegacySC::scrollDN(const s32 N) {
	vm.displayBuffer[0].shift(+N, 0);
}
void FunctionsForLegacySC::scrollLT(const s32) {
	vm.displayBuffer[0].shift(0, -4);
}
void FunctionsForLegacySC::scrollRT(const s32) {
	vm.displayBuffer[0].shift(0, +4);
}

/*------------------------------------------------------------------*/

usz FunctionsForLegacySC::bitBloat(usz byte) {
	if (!byte) { return 0; }
	byte = (byte << 4 | byte) & 0x0F0F;
	byte = (byte << 2 | byte) & 0x3333;
	byte = (byte << 1 | byte) & 0x5555;
	return  byte << 1 | byte;
}

void FunctionsForLegacySC::drawByte(
	s32 X, s32 Y,
	const usz DATA, bool& HIT
) {
	if (!DATA || X >= vm.Trait.W) { return; }

	for (auto B{ 0 }; B++ < 8; ++X &= vm.Trait.Wb)
	{
		if (DATA >> (8 - B) & 0x1) {
			auto& pixel{ vm.displayBuffer[0].at_raw(Y, X) };
			if (pixel) { HIT = true; }
			pixel ^= 1;
		}
		if (!vm.Quirk.wrapSprite && X == vm.Trait.Wb) { return; }
	}
}

void FunctionsForLegacySC::drawShort(
	s32 X, s32 Y,
	const usz DATA
) {
	if (!DATA) { return; }

	for (auto B{ 0 }; B++ < 16; ++X &= vm.Trait.Wb)
	{
		auto& pixel0{ vm.displayBuffer[0].at_raw(Y + 0, X) };
		auto& pixel1{ vm.displayBuffer[0].at_raw(Y + 1, X) };

		if (DATA >> (16 - B) & 0x1) {
			if (pixel0) { vm.mRegisterV[0xF] = 1; }
			pixel1 = pixel0 ^= 1;
		} else {
			pixel1 = pixel0;
		}
		if (!vm.Quirk.wrapSprite && X == vm.Trait.Wb) { return; }
	}
}

void FunctionsForLegacySC::drawSprite(
	s32 X, s32 Y, s32 N
) {
	auto VX{ vm.mRegisterV[X] };
	auto VY{ vm.mRegisterV[Y] };

	vm.mRegisterV[0xF] = 0;
	const bool wide{ N == 0 };
	if (wide) { N = 16; }

	if (vm.isLoresExtended()) {
		VX = VX * 2 & vm.Trait.Wb;
		VY = VY * 2 & vm.Trait.Hb;

		for (auto H{ 0 }, I{ 0 }; H < N; ++H, (VY += 2) &= vm.Trait.Hb)
		{
			drawShort(VX, VY, bitBloat(vm.readMemoryI(I++)));
			if (!vm.Quirk.wrapSprite && VY == vm.Trait.H - 2) { break; }
		}
	} else {
		VX &= vm.Trait.Wb;
		VY &= vm.Trait.Hb;

		for (auto H{ 0 }, I{ 0 }; H < N; ++H, ++VY &= vm.Trait.Hb)
		{
			bool ltHit{}, rtHit{};
			if (true) { drawByte(VX + 0, VY, vm.readMemoryI(I++), ltHit); }
			if (wide) { drawByte(VX + 8, VY, vm.readMemoryI(I++), rtHit); }
			vm.mRegisterV[0xF] += ltHit || rtHit;
			if (!vm.Quirk.wrapSprite && VY == vm.Trait.Hb) { break; }
		}
	}
}

void FunctionsForLegacySC::drawLoresColor(
	const s32 VX,
	const s32 VY,
	const s32 idx
) {
	if (vm.isLoresExtended()) {
		const auto H{ (VY & 0x77) << 0 }, maxH{ (H >> 4) + 1 };
		const auto W{ (VX & 0x77) << 1 }, maxW{ (W >> 4) + 2 };

		for (auto Y{ 0 }; Y < maxH; ++Y) {
			for (auto X{ 0 }; X < maxW; ++X) {
				vm.color8xBuffer.at_wrap(((H + Y) << 3) + 0, W + X) =
				vm.color8xBuffer.at_wrap(((H + Y) << 3) + 1, W + X) =
					vm.getForegroundColor8X(idx);
			}
		}
		vm.Trait.mask8X = 0xFC;
	}
	else {
		const auto H{ VY & 0x77 }, maxH{ (H >> 4) + 1 };
		const auto W{ VX & 0x77 }, maxW{ (W >> 4) + 1 };

		for (auto Y{ 0 }; Y < maxH; ++Y) {
			for (auto X{ 0 }; X < maxW; ++X) {
				vm.color8xBuffer.at_wrap(((H + Y) << 2), W + X) =
					vm.getForegroundColor8X(idx);
			}
		}
		vm.Trait.mask8X = 0xF8;
	}
}

void FunctionsForLegacySC::drawHiresColor(
	const s32 VX,
	const s32 VY,
	const s32 idx,
	const s32 N
) {
	if (vm.isLoresExtended()) {
		for (auto R{ 0 }, Y{ VY << 1 }, X{ VX << 1 >> 3 }; R < (N << 1); ++R) {
			vm.color8xBuffer.at_wrap(Y + R, X + 0) =
			vm.color8xBuffer.at_wrap(Y + R, X + 1) =
				vm.getForegroundColor8X(idx);
		}
	}
	else {
		for (auto R{ 0 }, Y{ VY }, X{ VX >> 3 }; R < N; ++R) {
			vm.color8xBuffer.at_wrap(Y + R, X) =
				vm.getForegroundColor8X(idx);
		}
	}
	vm.Trait.mask8X = 0xFF;
}
