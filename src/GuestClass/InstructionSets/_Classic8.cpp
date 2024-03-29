/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Interface.hpp"
#include "../Guest.hpp"
#include "../Registers.hpp"
#include "../MemoryBanks.hpp"
#include "../DisplayColors.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForClassic8                  */
/*------------------------------------------------------------------*/

FunctionsForClassic8::FunctionsForClassic8(VM_Guest* ref) : vm(ref) {}

void FunctionsForClassic8::scrollUP(const std::size_t N) {
	vm->State.push_display = true;
	auto& display{ vm->Mem->display };
	const auto N2{ vm->Plane.H - N };

	for (auto H{ 0 }; H < vm->Plane.H; ++H) 
	for (auto X{ 0 }; X < vm->Plane.X; ++X)
		display[H][X] = (H >= N2) ? 0 : display[H + N][X];
};
void FunctionsForClassic8::scrollDN(const std::size_t N) {
	vm->State.push_display = true;
	auto& display{ vm->Mem->display };

	for (auto H{ vm->Plane.Hb }; H >= 0; --H)
	for (auto X{ 0 }; X < vm->Plane.X; ++X)
		display[H][X] = (H < N) ? 0 : display[H - N][X];
};
void FunctionsForClassic8::scrollLT(const std::size_t) {
	vm->State.push_display = true;
	auto& display{ vm->Mem->display };

	for (auto H{ 0 }; H < vm->Plane.H; ++H)
	for (auto X{ 0 }; X < vm->Plane.X; ++X) {
		auto mask{ display[H][X] << 4 };
		if (X < vm->Plane.Xb)
			mask |= display[H][X + 1] >> 4;
		display[H][X] = static_cast<uint8_t>(mask);
	};
};
void FunctionsForClassic8::scrollRT(const std::size_t) {
	vm->State.push_display = true;
	auto& display{ vm->Mem->display };

	for (auto H{ 0 }; H < vm->Plane.H; ++H)
	for (auto X{ vm->Plane.Xb }; X >= 0; --X) {
		auto mask{ display[H][X] >> 4 };
		if (X > 0)
			mask |= display[H][X - 1] << 4;
		display[H][X] = static_cast<uint8_t>(mask);
	}
};

/*------------------------------------------------------------------*/

void FunctionsForClassic8::drawByte(
	const std::size_t L, const std::size_t SHL,
	const std::size_t R, const std::size_t SHR,
	const std::size_t Y, const std::size_t DATA
) {
	if (!DATA || L >= vm->Plane.X) return;
	const auto DATA_L{ DATA >> SHR & 0xFF };
	
	if (!vm->Reg->V[0xF]) [[unlikely]]
		vm->Reg->V[0xF]     = (vm->Mem->display[Y][L] & DATA_L) != 0;
	vm->Mem->display[Y][L] ^= DATA_L;

	if (!SHR || R >= vm->Plane.X) return;
	const auto DATA_R{ DATA << SHL & 0xFF };

	if (!vm->Reg->V[0xF]) [[unlikely]]
		vm->Reg->V[0xF]     = (vm->Mem->display[Y][R] & DATA_R) != 0;
	vm->Mem->display[Y][R] ^= DATA_R;
}

void FunctionsForClassic8::drawSprite(std::size_t VX, std::size_t VY, std::size_t N, std::size_t I) {
	vm->State.push_display = true;

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

	for (auto H{ VY }; H < N; ++H) {
		if (!vm->Quirk.wrapSprite)
			if (H >= vm->Plane.H) break;

		const auto Y{ H & vm->Plane.Hb };

		drawByte(X0, SHL, X1, SHR, Y, vm->mrw(I++));
		if (!wide) continue;
		drawByte(X1, SHL, X2, SHR, Y, vm->mrw(I++));
	}
};

void FunctionsForClassic8::drawColors(const std::size_t VX, const std::size_t VY, const std::size_t idx, const std::size_t N) {
	vm->State.push_display = true;

	if (N) {
		const auto X{ VX >> 3 };
		for (auto _Y{ 0 }; _Y < N; ++_Y) {
			const auto Y{ VY + _Y & vm->Plane.Hb };
			vm->Mem->bufColor8x[Y][X] = vm->Color->getFore8X(idx);
		}
		vm->State.chip8X_hires = true;
	}
	else {
		const auto H{ (VY >> 4u) + 1u };
		const auto W{ (VX >> 4u) + 1u };

		for (auto _Y{ 0 }; _Y < H; ++_Y) {
			const auto Y{ ((VY + _Y) << 2) & vm->Plane.Hb };
			for (auto _X{ 0 }; _X < W; ++_X) {
				const auto X{ VX + _X & vm->Plane.Xb };
				vm->Mem->bufColor8x[Y][X] = vm->Color->getFore8X(idx);
			}
		}
		vm->State.chip8X_hires = false;
	}
};
