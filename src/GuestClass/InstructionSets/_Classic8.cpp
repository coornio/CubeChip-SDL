/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <utility>

#include "Interface.hpp"
#include "../Guest.hpp"
#include "../MemoryBanks.hpp"
#include "../DisplayTraits.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForClassic8                  */
/*------------------------------------------------------------------*/

FunctionsForClassic8::FunctionsForClassic8(VM_Guest* parent) noexcept
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
	MemoryBanks* Mem, DisplayTraits* Display,
	std::int32_t X, std::int32_t Y,
	const std::size_t DATA
) {
	if (!DATA) { return; }
	if (vm->Quirk.wrapSprite) { X &= Display->Trait.Wb; }
	else if (X >= Display->Trait.W) { return; }

	for (auto B{ 0 }; B++ < 8; ++X &= Display->Trait.Wb) {
		if (DATA >> (8 - B) & 0x1) {
			auto& elem{ Mem->displayBuffer[0].at_raw(Y, X) };
			if (elem) { Mem->vRegister[0xF] = 1; }
			elem ^= 1;
		}
		if (!vm->Quirk.wrapSprite && X == Display->Trait.Wb) { return; }
	}
}

void FunctionsForClassic8::drawSprite(
	MemoryBanks*   const Mem,
	DisplayTraits* const Display,
	std::int32_t X,
	std::int32_t Y,
	std::int32_t N
) {
	X = Mem->vRegister[X] & Display->Trait.Wb;
	Y = Mem->vRegister[Y] & Display->Trait.Hb;

	Mem->vRegister[0xF] = 0;

	const bool wide{ N == 0 };
	if (wide) { N = 16; }

	for (auto H{ 0 }, I{ 0 }; H < N; ++H, ++Y &= Display->Trait.Hb)
	{
		if (true) { drawByte(Mem, Display, X + 0, Y, Mem->read_idx(I++)); }
		if (wide) { drawByte(Mem, Display, X + 8, Y, Mem->read_idx(I++)); }
		if (!vm->Quirk.wrapSprite && Y == Display->Trait.Hb) { break; }
	}
}

void FunctionsForClassic8::drawLoresColor(
	MemoryBanks* Mem, DisplayTraits* Display,
	const std::int32_t VX,
	const std::int32_t VY,
	const std::int32_t idx
) {
	for (auto Y{ 0 }, maxH{ VY >> 4 }; Y <= maxH; ++Y) {
		for (auto X{ 0 }, maxW{ VX >> 4 }; X <= maxW; ++X) {
			Mem->color8xBuffer.at_wrap((VY + Y) << 2, VX + X) = 
				Display->Color.getFore8X(idx);
		}
	}
	Display->Trait.mask8X = 0xFC;
}

void FunctionsForClassic8::drawHiresColor(
	MemoryBanks* Mem, DisplayTraits* Display,
	const std::int32_t VX,
	const std::int32_t VY,
	const std::int32_t idx,
	const std::int32_t N
) {
	for (auto Y{ VY }, X{ VX >> 3 }; Y < VY + N; ++Y) {
		Mem->color8xBuffer.at_wrap(Y, X) =
			Display->Color.getFore8X(idx);
	}
	Display->Trait.mask8X = 0xFF;
}
