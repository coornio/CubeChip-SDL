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
	std::int32_t X, std::int32_t Y,
	const std::size_t DATA, bool& HIT
) {
	if (!DATA || X >= vm->Plane.W) { return; }

	for (auto B{ 0 }; B++ < 8; ++X &= vm->Plane.Wb)
	{
		if (DATA >> (8 - B) & 0x1) {
			auto& pixel{ vm->Mem->displayBuffer[0].at_raw(Y, X) };
			if (pixel) { HIT = true; }
			pixel ^= 1;
		}
		if (!vm->Quirk.wrapSprite && X == vm->Plane.Wb) { return; }
	}
}

void FunctionsForLegacySC::drawShort(
	std::int32_t X, std::int32_t Y,
	const std::size_t DATA
) {
	if (!DATA) { return; }

	for (auto B{ 0 }; B++ < 16; ++X &= vm->Plane.Wb)
	{
		auto& pixel0{ vm->Mem->displayBuffer[0].at_raw(Y + 0, X) };
		auto& pixel1{ vm->Mem->displayBuffer[0].at_raw(Y + 1, X) };

		if (DATA >> (16 - B) & 0x1) {
			if (pixel0) { vm->Reg->V[0xF] = 1; }
			pixel1 = pixel0 ^= 1;
		} else {
			pixel1 = pixel0;
		}
		if (!vm->Quirk.wrapSprite && X == vm->Plane.Wb) { return; }
	}
}

void FunctionsForLegacySC::drawSprite(
	std::int32_t  VX, std::int32_t  VY,
	std::int32_t   N, std::uint32_t IR
) {
	vm->Reg->V[0xF] = 0;
	const bool wide{ N == 0 };
	if (wide) { N = 16; }

	if (vm->Program->screenLores) {
		VX = VX * 2 & vm->Plane.Wb;
		VY = VY * 2 & vm->Plane.Hb;

		for (auto H{ 0 }; H < N; ++H, (VY += 2) &= vm->Plane.Hb)
		{
			drawShort(VX, VY, bitBloat(vm->mrw(IR++)));
			if (!vm->Quirk.wrapSprite && VY == vm->Plane.H - 2) { break; }
		}
	} else {
		VX &= vm->Plane.Wb;
		VY &= vm->Plane.Hb;

		for (auto H{ 0 }; H < N; ++H, ++VY &= vm->Plane.Hb)
		{
			bool ltHit{}, rtHit{};
			if (true) { drawByte(VX + 0, VY, vm->mrw(IR++), ltHit); }
			if (wide) { drawByte(VX + 8, VY, vm->mrw(IR++), rtHit); }
			vm->Reg->V[0xF] += ltHit || rtHit;
			if (!vm->Quirk.wrapSprite && VY == vm->Plane.Hb) { break; }
		}
	}
}

void FunctionsForLegacySC::drawLoresColor(
	const std::int32_t VX,
	const std::int32_t VY,
	const std::int32_t idx
) {
	if (vm->Program->screenLores) {
		const auto H{ (VY & 0x77) << 0 }, maxH{ (H >> 4) + 1 };
		const auto W{ (VX & 0x77) << 1 }, maxW{ (W >> 4) + 2 };

		for (auto Y{ 0 }; Y < maxH; ++Y) {
			for (auto X{ 0 }; X < maxW; ++X) {
				vm->Mem->color8xBuffer.at_wrap(((H + Y) << 3) + 0, W + X) =
				vm->Mem->color8xBuffer.at_wrap(((H + Y) << 3) + 1, W + X) =
					vm->Color->getFore8X(idx);
			}
		}
	}
	else {
		const auto H{ VY & 0x77 }, maxH{ (H >> 4) + 1 };
		const auto W{ VX & 0x77 }, maxW{ (W >> 4) + 1 };

		for (auto Y{ 0 }; Y < maxH; ++Y) {
			for (auto X{ 0 }; X < maxW; ++X) {
				vm->Mem->color8xBuffer.at_wrap(((H + Y) << 2), W + X) =
					vm->Color->getFore8X(idx);
			}
		}
	}
}

void FunctionsForLegacySC::drawHiresColor(
	const std::int32_t VX,
	const std::int32_t VY,
	const std::int32_t idx,
	const std::int32_t N
) {
	if (vm->Program->screenLores) {
		for (auto R{ 0 }, Y{ VY << 1 }, X{ VX << 1 >> 3 }; R < (N << 1); ++R) {
			vm->Mem->color8xBuffer.at_wrap(Y + R, X + 0) =
			vm->Mem->color8xBuffer.at_wrap(Y + R, X + 1) =
				vm->Color->getFore8X(idx);
		}
	}
	else {
		for (auto R{ 0 }, Y{ VY }, X{ VX >> 3 }; R < N; ++R) {
			vm->Mem->color8xBuffer.at_wrap(Y + R, X) =
				vm->Color->getFore8X(idx);
		}
	}
}
