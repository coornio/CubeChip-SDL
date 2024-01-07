
#include "../GuestClass/Guest.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForClassic8                  */
/*------------------------------------------------------------------*/

void FunctionsForClassic8::scrollUP(const s32 N) {
	vm.State.push_display = true;
	auto& display{ vm.Mem.display };
	const auto N2{ vm.Plane.H - N };

	for (auto H{ 0 }; H < vm.Plane.H; ++H) 
	for (auto X{ 0 }; X < vm.Plane.X; ++X)
		display[H][X] = (H >= N2) ? 0 : display[H + N][X];
};
void FunctionsForClassic8::scrollDN(const s32 N) {
	vm.State.push_display = true;
	auto& display{ vm.Mem.display };

	for (auto H{ vm.Plane.Hb }; H >= 0; --H)
	for (auto X{ 0 }; X < vm.Plane.X; ++X)
		display[H][X] = (H < N) ? 0 : display[H - N][X];
};
void FunctionsForClassic8::scrollLT(s32) {
	vm.State.push_display = true;
	auto& display{ vm.Mem.display };

	for (auto H{ 0 }; H < vm.Plane.H; ++H)
	for (auto X{ 0 }; X < vm.Plane.X; ++X) {
		auto mask{ display[H][X] << 4 };
		if (X < vm.Plane.Xb)
			mask |= display[H][X + 1] >> 4;
		display[H][X] = as<u8>(mask);
	};
};
void FunctionsForClassic8::scrollRT(s32) {
	vm.State.push_display = true;
	auto& display{ vm.Mem.display };

	for (auto H{ 0 }; H < vm.Plane.H; ++H)
	for (auto X{ vm.Plane.Xb }; X >= 0; --X) {
		auto mask{ display[H][X] >> 4 };
		if (X > 0)
			mask |= display[H][X - 1] << 4;
		display[H][X] = as<u8>(mask);
	}
};

/*------------------------------------------------------------------*/

void FunctionsForClassic8::drawByte(
	const s32 L, const s32 SHL,
	const s32 R, const s32 SHR,
	const s32 Y, const u8  DATA
) {
	if (!DATA || L >= vm.Plane.X) return;
	const auto DATA_L{ as<u8>(DATA >> SHR) };
	
	if (!vm.Reg.V[0xF]) [[unlikely]]
		vm.Reg.V[0xF]     = (vm.Mem.display[Y][L] & DATA_L) != 0;
	vm.Mem.display[Y][L] ^= DATA_L;

	if (!SHR || R >= vm.Plane.X) return;
	const auto DATA_R{ as<u8>(DATA << SHL) };

	if (!vm.Reg.V[0xF]) [[unlikely]]
		vm.Reg.V[0xF]     = (vm.Mem.display[Y][R] & DATA_R) != 0;
	vm.Mem.display[Y][R] ^= DATA_R;
}



void FunctionsForClassic8::drawSprite(u8 VX, u8 VY, s32 N, u32 I) {
	vm.State.push_display = true;

	VX &= vm.Plane.Wb;
	VY &= vm.Plane.Hb;

	const bool wide{ N == 0 };
	N = VY + (wide ? 16 : N);

	const auto SHR{ VX & 7 };
	const auto SHL{ 8 - SHR };

	auto X0{ VX >> 3 };
	auto X1{ X0 + 1 };
	auto X2{ X0 + 2 };

	if (vm.Quirk.wrapSprite) {
		X1 &= vm.Plane.Xb;
		X2 &= vm.Plane.Xb;
	}

	for (auto H{ VY }; H < N; ++H) {
		if (!vm.Quirk.wrapSprite)
			if (H >= vm.Plane.H) break;

		const auto Y{ H & vm.Plane.Hb };

		drawByte(X0, SHL, X1, SHR, Y, vm.mrw(I++));
		if (!wide) continue;
		drawByte(X1, SHL, X2, SHR, Y, vm.mrw(I++));
	}
};

void FunctionsForClassic8::drawColors(u8 VX, u8 VY, u8 idx, s32 N) {
	vm.State.push_display = true;

	if (N) {
		const auto X{ VX >> 3 };
		for (auto _Y{ 0 }; _Y < N; ++_Y) {
			const auto Y{ VY + _Y & vm.Plane.Hb };
			vm.Mem.bufColor8x[Y][X] = vm.Color.getFore8X(idx);
		}
		vm.State.chip8X_hires = true;
	}
	else {
		const auto H{ (VY >> 4) + 1 };
		const auto W{ (VX >> 4) + 1 };

		for (auto _Y{ 0 }; _Y < H; ++_Y) {
			const auto Y{ ((VY + _Y) << 2) & vm.Plane.Hb };
			for (auto _X{ 0 }; _X < W; ++_X) {
				const auto X{ VX + _X & vm.Plane.Xb };
				vm.Mem.bufColor8x[Y][X] = vm.Color.getFore8X(idx);
			}
		}
		vm.State.chip8X_hires = false;
	}
};
