/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Interface.hpp"
#include "../Guest.hpp"
#include "../DisplayTraits.hpp"
#include "../ProgramControl.hpp"
#include "../MemoryBanks.hpp"
#include "../DisplayTraits.hpp"

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
	if (!byte) { return 0; }
	byte = (byte << 4 | byte) & 0x0F0F;
	byte = (byte << 2 | byte) & 0x3333;
	byte = (byte << 1 | byte) & 0x5555;
	return  byte << 1 | byte;
}

void FunctionsForLegacySC::drawByte(
	MemoryBanks* Mem, DisplayTraits* Display,
	std::int32_t X, std::int32_t Y,
	const std::size_t DATA, bool& HIT
) {
	if (!DATA || X >= Display->Trait.W) { return; }

	for (auto B{ 0 }; B++ < 8; ++X &= Display->Trait.Wb)
	{
		if (DATA >> (8 - B) & 0x1) {
			auto& pixel{ Mem->displayBuffer[0].at_raw(Y, X) };
			if (pixel) { HIT = true; }
			pixel ^= 1;
		}
		if (!vm->Quirk.wrapSprite && X == Display->Trait.Wb) { return; }
	}
}

void FunctionsForLegacySC::drawShort(
	MemoryBanks* Mem, DisplayTraits* Display,
	std::int32_t X, std::int32_t Y,
	const std::size_t DATA
) {
	if (!DATA) { return; }

	for (auto B{ 0 }; B++ < 16; ++X &= Display->Trait.Wb)
	{
		auto& pixel0{ Mem->displayBuffer[0].at_raw(Y + 0, X) };
		auto& pixel1{ Mem->displayBuffer[0].at_raw(Y + 1, X) };

		if (DATA >> (16 - B) & 0x1) {
			if (pixel0) { Mem->vRegister[0xF] = 1; }
			pixel1 = pixel0 ^= 1;
		} else {
			pixel1 = pixel0;
		}
		if (!vm->Quirk.wrapSprite && X == Display->Trait.Wb) { return; }
	}
}

void FunctionsForLegacySC::drawSprite(
	MemoryBanks* Mem, DisplayTraits* Display,
	std::int32_t X, std::int32_t Y, std::int32_t N
) {
	auto VX{ Mem->vRegister[X] };
	auto VY{ Mem->vRegister[Y] };

	Mem->vRegister[0xF] = 0;
	const bool wide{ N == 0 };
	if (wide) { N = 16; }

	if (vm->Display->isLoresExtended()) {
		VX = VX * 2 & Display->Trait.Wb;
		VY = VY * 2 & Display->Trait.Hb;

		for (auto H{ 0 }, I{ 0 }; H < N; ++H, (VY += 2) &= Display->Trait.Hb)
		{
			drawShort(Mem, Display, VX, VY, bitBloat(Mem->read_idx(I++)));
			if (!vm->Quirk.wrapSprite && VY == Display->Trait.H - 2) { break; }
		}
	} else {
		VX &= Display->Trait.Wb;
		VY &= Display->Trait.Hb;

		for (auto H{ 0 }, I{ 0 }; H < N; ++H, ++VY &= Display->Trait.Hb)
		{
			bool ltHit{}, rtHit{};
			if (true) { drawByte(Mem, Display, VX + 0, VY, Mem->read_idx(I++), ltHit); }
			if (wide) { drawByte(Mem, Display, VX + 8, VY, Mem->read_idx(I++), rtHit); }
			Mem->vRegister[0xF] += ltHit || rtHit;
			if (!vm->Quirk.wrapSprite && VY == Display->Trait.Hb) { break; }
		}
	}
}

void FunctionsForLegacySC::drawLoresColor(
	MemoryBanks* Mem, DisplayTraits* Display,
	const std::int32_t VX,
	const std::int32_t VY,
	const std::int32_t idx
) {
	if (Display->isLoresExtended()) {
		const auto H{ (VY & 0x77) << 0 }, maxH{ (H >> 4) + 1 };
		const auto W{ (VX & 0x77) << 1 }, maxW{ (W >> 4) + 2 };

		for (auto Y{ 0 }; Y < maxH; ++Y) {
			for (auto X{ 0 }; X < maxW; ++X) {
				Mem->color8xBuffer.at_wrap(((H + Y) << 3) + 0, W + X) =
				Mem->color8xBuffer.at_wrap(((H + Y) << 3) + 1, W + X) =
					Display->Color.getFore8X(idx);
			}
		}
		Display->Trait.mask8X = 0xFC;
	}
	else {
		const auto H{ VY & 0x77 }, maxH{ (H >> 4) + 1 };
		const auto W{ VX & 0x77 }, maxW{ (W >> 4) + 1 };

		for (auto Y{ 0 }; Y < maxH; ++Y) {
			for (auto X{ 0 }; X < maxW; ++X) {
				Mem->color8xBuffer.at_wrap(((H + Y) << 2), W + X) =
					Display->Color.getFore8X(idx);
			}
		}
		Display->Trait.mask8X = 0xF8;
	}
}

void FunctionsForLegacySC::drawHiresColor(
	MemoryBanks* Mem, DisplayTraits* Display,
	const std::int32_t VX,
	const std::int32_t VY,
	const std::int32_t idx,
	const std::int32_t N
) {
	if (Display->isLoresExtended()) {
		for (auto R{ 0 }, Y{ VY << 1 }, X{ VX << 1 >> 3 }; R < (N << 1); ++R) {
			Mem->color8xBuffer.at_wrap(Y + R, X + 0) =
			Mem->color8xBuffer.at_wrap(Y + R, X + 1) =
				Display->Color.getFore8X(idx);
		}
	}
	else {
		for (auto R{ 0 }, Y{ VY }, X{ VX >> 3 }; R < N; ++R) {
			Mem->color8xBuffer.at_wrap(Y + R, X) =
				Display->Color.getFore8X(idx);
		}
	}
	Display->Trait.mask8X = 0xFF;
}
