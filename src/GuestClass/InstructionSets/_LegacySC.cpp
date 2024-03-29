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

FunctionsForLegacySC::FunctionsForLegacySC(VM_Guest& ref) : vm(ref) {}

void FunctionsForLegacySC::scrollUP(const std::size_t N) {
	vm.State.push_display = true;
	auto& display{ vm.Mem.display };
	const auto N2{ vm.Plane.H - N };

	for (auto H{ 0 }; H < vm.Plane.H; ++H)
	for (auto X{ 0 }; X < vm.Plane.X; ++X)
		display[H][X] = (H >= N2) ? 0 : display[H + N][X];
};
void FunctionsForLegacySC::scrollDN(const std::size_t N) {
	vm.State.push_display = true;
	auto& display{ vm.Mem.display };

	for (auto H{ vm.Plane.Hb }; H >= 0; --H)
	for (auto X{ 0 }; X < vm.Plane.X; ++X)
		display[H][X] = (H < N) ? 0 : display[H - N][X];
};
void FunctionsForLegacySC::scrollLT(const std::size_t) {
	vm.State.push_display = true;
	auto& display{ vm.Mem.display };

	for (auto H{ 0 }; H < vm.Plane.H; ++H)
	for (auto X{ 0 }; X < vm.Plane.X; ++X) {
		auto mask{ display[H][X] << 4 };
		if (X < vm.Plane.Xb)
			mask |= display[H][X + 1] >> 4;
		display[H][X] = static_cast<uint8_t>(mask);
	}
};
void FunctionsForLegacySC::scrollRT(const std::size_t) {
	vm.State.push_display = true;
	auto& display{ vm.Mem.display };

	for (auto H{ 0 }; H < vm.Plane.H; ++H)
	for (auto X{ vm.Plane.Xb }; X >= 0; --X) {
		auto mask{ display[H][X] >> 4 };
		if (X > 0)
			mask |= display[H][X - 1] << 4;
		display[H][X] = static_cast<uint8_t>(mask);
	}
};

/*------------------------------------------------------------------*/

std::size_t  FunctionsForLegacySC::bitBloat(std::size_t byte) {
	if (!byte) return 0;
	byte = (byte << 4 | byte) & 0x0F0F;
	byte = (byte << 2 | byte) & 0x3333;
	byte = (byte << 1 | byte) & 0x5555;
	return  byte << 1 | byte;
}

void FunctionsForLegacySC::drawByte(
	const std::size_t L, const std::size_t SHL,
	const std::size_t R, const std::size_t SHR,
	const std::size_t Y, const std::size_t DATA
) {
	if (!DATA || L >= vm.Plane.X) return;
	const auto DATA_L{ DATA >> SHR & 0xFF };

	vm.Reg.V[0xF]        += (DATA_L & vm.Mem.display[Y][L]) != 0;
	vm.Mem.display[Y][L] ^=  DATA_L;

	if (!SHR || R >= vm.Plane.X) return;
	const auto DATA_R{ DATA << SHL & 0xFF };

	vm.Reg.V[0xF]        += (DATA_R & vm.Mem.display[Y][R]) != 0;
	vm.Mem.display[Y][R] ^=  DATA_R;
}

void FunctionsForLegacySC::drawShort(
	const std::size_t L, const std::size_t SHL,
	const std::size_t R, const std::size_t SHR,
	const std::size_t Y, const std::size_t DATA
) {
	if (!DATA || L >= vm.Plane.X) return;
	const auto DATA_L{ DATA >> SHR & 0xFF };
	
	if (!vm.Reg.V[0xF]) [[unlikely]]
		vm.Reg.V[0xF]        = (vm.Mem.display[Y][L]  & DATA_L) != 0;
	vm.Mem.display[Y + 1][L] =  vm.Mem.display[Y][L] ^= DATA_L;

	if (!SHR || R >= vm.Plane.X) return;
	const auto DATA_R{ DATA << SHL & 0xFF };

	if (!vm.Reg.V[0xF]) [[unlikely]]
		vm.Reg.V[0xF]        = (vm.Mem.display[Y][R]  & DATA_R) != 0;
	vm.Mem.display[Y + 1][R] =  vm.Mem.display[Y][R] ^= DATA_R;
}

void FunctionsForLegacySC::drawSprite(std::size_t VX, std::size_t VY, std::size_t N, std::size_t I) {
	vm.State.push_display = true;
	const auto mode{ vm.Program.screenMode };

	VX *= mode; VX &= vm.Plane.Wb;
	VY *= mode; VY &= vm.Plane.Hb;
	vm.Reg.V[0xF] = 0;

	const bool wide{ N == 0 };
	N = VY + (wide ? 16 : N) * mode;

	const auto SHR{ VX & 7 };
	const auto SHL{ 8 - SHR };

	auto X0{ VX >> 3 };
	auto X1{ X0 + 1 };
	auto X2{ X0 + 2 };

	if (vm.Quirk.wrapSprite) {
		X1 &= vm.Plane.Xb;
		X2 &= vm.Plane.Xb;
	}

	for (auto H{ VY }; H < N; H += mode) {
		if (!vm.Quirk.wrapSprite)
			if (H >= vm.Plane.H) break;
		const auto Y{ H & vm.Plane.Hb };

		if (mode == vm.Resolution::LO) { // lores 8xN (doubled)
			const auto DATA{ bitBloat(vm.mrw(I++)) & 0xFFFFu };
			drawShort(X0, SHL, X1, SHR, Y, DATA >> 0x8u);
			drawShort(X1, SHL, X2, SHR, Y, DATA & 0xFFu);
		} else {						 // hires 8xN / 16xN
			drawByte(X0, SHL, X1, SHR, Y, vm.mrw(I++));
			if (!wide) continue;
			drawByte(X1, SHL, X2, SHR, Y, vm.mrw(I++));
		}
	}
};

void FunctionsForLegacySC::drawColors(std::size_t VX, std::size_t VY, std::size_t idx, std::size_t N) {
	vm.State.push_display = true;
	auto mode{ vm.Program.screenMode };

	if (N) {
		if (mode == vm.Resolution::LO)
			VY <<= 1, VX <<= 1, N <<= 1;

		const auto X{ VX >> 3 };
		for (auto _Y{ 0 }; _Y < N; ++_Y) {
			const auto Y{ VY + _Y & vm.Plane.Hb };
			vm.Mem.bufColor8x[Y][X + 0] = vm.Color.getFore8X(idx);
			if (mode != vm.Resolution::LO) continue;
			vm.Mem.bufColor8x[Y][X + 1] = vm.Color.getFore8X(idx);
		}
		vm.State.chip8X_hires = true;
	}
	else {
		VY &= 0x77, VX &= 0x77;

		if (mode == vm.Resolution::LO)
			VY <<= 1, VX <<= 1;

		const auto H{ (VY >> 4) + mode };
		const auto W{ (VX >> 4) + mode };

		for (auto _Y{ 0 }; _Y < H; ++_Y) {
			const auto Y{ ((VY + _Y) << 2) & vm.Plane.Hb };
			for (auto _X{ 0 }; _X < W; ++_X) {
				const auto X{ VX + _X & vm.Plane.Xb };
				vm.Mem.bufColor8x[Y + 0][X] = vm.Color.getFore8X(idx);
				//if (mode != vm.Resolution::LO) continue;
				//vm.Mem.bufColor8x[Y + 1][X] = vm.Color.getFore8X(idx);
			}
		}
		vm.State.chip8X_hires = false;
	}
};
