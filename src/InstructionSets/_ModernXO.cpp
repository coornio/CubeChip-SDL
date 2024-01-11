/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../GuestClass/Guest.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForModernXO                  */
/*------------------------------------------------------------------*/

void FunctionsForModernXO::scrollUP(const s32 N) {
	if (!vm.Plane.selected) return;
	vm.State.push_display = true;
	auto& display{ vm.Mem.display };
	const auto N2{ vm.Plane.H - N };

	for (auto H{ 0 }; H < vm.Plane.H; ++H)
	for (auto X{ 0 }; X < vm.Plane.X; ++X) {
		display[H][X] &= ~vm.Plane.mask;
		if (H >= N2) continue;
		display[H][X] |= vm.Plane.mask & display[H + N][X];
	}
};
void FunctionsForModernXO::scrollDN(const s32 N) {
	if (!vm.Plane.selected) return;
	vm.State.push_display = true;
	auto& display{ vm.Mem.display };

	for (auto H{ vm.Plane.Hb }; H >= 0; --H)
	for (auto X{ 0 }; X < vm.Plane.X; ++X) {
		display[H][X] &= ~vm.Plane.mask;
		if (H < N) continue;
		display[H][X] |= vm.Plane.mask & display[H - N][X];
	}
};
void FunctionsForModernXO::scrollLT(const s32) {
	if (!vm.Plane.selected) return;
	vm.State.push_display = true;
	auto& display{ vm.Mem.display };

	for (auto H{ 0 }; H < vm.Plane.H; ++H)
	for (auto X{ 0 }; X < vm.Plane.X; ++X) {
		auto mask{ display[H][X] << 16 };
		if (X < vm.Plane.Xb)
			mask |= display[H][X + 1] >> 16;
		display[H][X] &= ~vm.Plane.mask;
		display[H][X] |=  vm.Plane.mask & mask;
	}
};
void FunctionsForModernXO::scrollRT(const s32) {
	if (!vm.Plane.selected) return;
	vm.State.push_display = true;
	auto& display{ vm.Mem.display };

	for (auto H{ 0 }; H < vm.Plane.H; ++H)
	for (auto X{ vm.Plane.Xb }; X >= 0; --X) {
		auto mask{ display[H][X] >> 16 };
		if (X > 0)
			mask |= display[H][X - 1] << 16;
		display[H][X] &= ~vm.Plane.mask;
		display[H][X] |=  vm.Plane.mask & mask;
	}
};

/*------------------------------------------------------------------*/

void FunctionsForModernXO::applyBrush(u32& addr, const u32 data) {
	switch (vm.Plane.brush) {
		case VM_Guest::BrushType::XOR: addr ^=  data; return;
		case VM_Guest::BrushType::SUB: addr &= ~data; return;
		case VM_Guest::BrushType::ADD: addr |=  data; return;
	}
}

u32 FunctionsForModernXO::bitBloat(u32 byte) {
	if (!byte) return 0;
	byte = (byte << 12 | byte) & 0x000F000F;
	byte = (byte <<  6 | byte) & 0x03030303;
	return (byte <<  3 | byte) & 0x11111111;
}

void FunctionsForModernXO::drawByte(
	const s32 L, const s32 SHL,
	const s32 R, const s32 SHR,
	const s32 Y, const u32 DATA
) {
	if (!DATA || L >= vm.Plane.X) return;
	const auto DATA_L{ DATA >> SHR };

	if (!vm.Reg.V[0xF]) [[unlikely]]
		vm.Reg.V[0xF]  = (DATA_L & vm.Mem.display[Y][L]) != 0;
	applyBrush(vm.Mem.display[Y][L], DATA_L);

	if (!SHR || R >= vm.Plane.X) return;
	const auto DATA_R{ DATA << SHL };

	if (!vm.Reg.V[0xF]) [[unlikely]]
		vm.Reg.V[0xF]  = (DATA_R & vm.Mem.display[Y][R]) != 0;
	applyBrush(vm.Mem.display[Y][R], DATA_R);
}

void FunctionsForModernXO::drawSprite(u8 VX, u8 VY, s32 N, u32 I) {
	if (!vm.Plane.selected) return;
	vm.State.push_display = true;

	VX &= vm.Plane.Wb;
	VY &= vm.Plane.Hb;

	const bool wide{ N == 0 };
	N = VY + (wide ? 16 : N);

	const auto SHR{ (VX & 7) << 2 };
	const auto SHL{ 32 - SHR };

	auto X0{ VX >> 3 };
	auto X1{ X0 + 1 };
	auto X2{ X0 + 2 };

	if (vm.Quirk.wrapSprite) {
		X1 &= vm.Plane.Xb;
		X2 &= vm.Plane.Xb;
	}

	for (auto bitplane{ 1 }; bitplane <= 0x8; bitplane <<= 1) {
		if (!(bitplane & vm.Plane.selected)) continue;

		for (auto H{ VY }; H < N; ++H) {
			if (!vm.Quirk.wrapSprite)
				if (H >= vm.Plane.H) break;

			const auto Y{ H & vm.Plane.Hb };

			drawByte(X0, SHL, X1, SHR, Y, bitBloat(vm.mrw(I++)) * bitplane);
			if (!wide) continue;
			drawByte(X1, SHL, X2, SHR, Y, bitBloat(vm.mrw(I++)) * bitplane);
		}
	}
};
