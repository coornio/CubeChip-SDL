/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>
#include <algorithm>
#include <execution>

#include "../../HostClass/BasicVideoSpec.hpp"

#include "Interface.hpp"
#include "../Guest.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForMegachip                  */
/*------------------------------------------------------------------*/

FunctionsForMegachip::FunctionsForMegachip(VM_Guest& parent) noexcept
	: vm{ parent }
{
	chooseBlend(Blend::NORMAL);
}

struct ColorF { f32 A, R, G, B; };

/*------------------------------------------------------------------*/

void FunctionsForMegachip::scrollUP(const s32 N) {
	vm.foregroundBuffer.shift(-N, 0);
	blendBuffersToTexture(
		vm.foregroundBuffer.span(),
		vm.backgroundBuffer.span()
	);
}
void FunctionsForMegachip::scrollDN(const s32 N) {
	vm.foregroundBuffer.shift(+N, 0);
	blendBuffersToTexture(
		vm.foregroundBuffer.span(),
		vm.backgroundBuffer.span()
	);
}
void FunctionsForMegachip::scrollLT(const s32 N) {
	vm.foregroundBuffer.shift(0, -N);
	blendBuffersToTexture(
		vm.foregroundBuffer.span(),
		vm.backgroundBuffer.span()
	);
}
void FunctionsForMegachip::scrollRT(const s32 N) {
	vm.foregroundBuffer.shift(0, +N);
	blendBuffersToTexture(
		vm.foregroundBuffer.span(),
		vm.backgroundBuffer.span()
	);
}

/*------------------------------------------------------------------*/

static u32 blendPixel(
	const u32 srcPixel, const u32 dstPixel, const f32 alpha,
	f32(*blend)(const f32, const f32)
) noexcept {
	static constexpr f32 minF{ 1.0f / 255.0f };
	ColorF src, dst;

	src.A =  (srcPixel >> 24) * alpha * minF;
	if (src.A < minF) [[unlikely]] { return dstPixel; }
	src.R = ((srcPixel >> 16) & 0xFF) * minF;
	src.G = ((srcPixel >>  8) & 0xFF) * minF;
	src.B =  (srcPixel        & 0xFF) * minF;

	dst.A =  (dstPixel >> 24)         * minF;
	dst.R = ((dstPixel >> 16) & 0xFF) * minF;
	dst.G = ((dstPixel >>  8) & 0xFF) * minF;
	dst.B =  (dstPixel        & 0xFF) * minF;

	auto R{ blend(src.R, dst.R) };
	auto G{ blend(src.G, dst.G) };
	auto B{ blend(src.B, dst.B) };

	if (src.A < 1.0f) {
		const f32 sW{ src.A / 1.0f };
		const f32 dW{ 1.0f - sW };

		R = dW * dst.R + sW * R;
		G = dW * dst.G + sW * G;
		B = dW * dst.B + sW * B;
	}

	return 0xFF000000
		| static_cast<u8>(std::roundf(R * 255.0f)) << 16
		| static_cast<u8>(std::roundf(G * 255.0f)) <<  8
		| static_cast<u8>(std::roundf(B * 255.0f));
}

void FunctionsForMegachip::blendBuffersToTexture(
	const std::span<const u32> srcColors,
	const std::span<const u32> dstColors
) {
	std::transform(
		std::execution::par_unseq,
		srcColors.begin(), srcColors.end(),
		dstColors.begin(),
		vm.BVS.lockTexture(),
		[this](const u32 src, const u32 dst) {
			return blendPixel(src, dst, vm.Texture.alpha, blendAlgo);
		}
	);
	vm.BVS.unlockTexture();
}

void FunctionsForMegachip::drawSprite(
	s32 _X, s32 _Y, s32 FR
) {
	s32 VX{ vm.mRegisterV[_X] };
	s32 VY{ vm.mRegisterV[_Y] };

	vm.mRegisterV[0xF] = 0;
	if (!vm.Quirk.wrapSprite && VY >= vm.Trait.H) { return; }
	if (vm.mRegisterI >= 0xF0) [[likely]] { goto paintTexture; }

	for (auto H{ 0 }, Y{ VY }; H < FR; ++H, ++Y &= vm.Trait.Wb)
	{
		if (vm.Quirk.wrapSprite && Y >= vm.Trait.H) { continue; }
		const auto bytePixel{ vm.readMemoryI(H) };

		for (auto W{ 7 }, X{ VX }; W >= 0; --W, ++X &= vm.Trait.Wb)
		{
			if (bytePixel >> W & 0x1)
			{
				auto& collideCoord{ vm.collisionPalette.at_raw(Y, X) };
				auto& backbufCoord{ vm.backgroundBuffer.at_raw(Y, X) };

				if (collideCoord) [[unlikely]] {
					collideCoord = 0;
					backbufCoord = 0;
					vm.mRegisterV[0xF] = 1;
				} else {
					collideCoord = 254;
					backbufCoord = vm.Color.hex[H];
				}
			}
			if (!vm.Quirk.wrapSprite && X == vm.Trait.Wb) { break; }
		}
		if (!vm.Quirk.wrapSprite && Y == vm.Trait.Hb) { break; }
	}
	return;

paintTexture:
	for (auto H{ 0 }, Y{ VY }; H < vm.Texture.H; ++H, ++Y &= vm.Trait.Wb)
	{
		if (vm.Quirk.wrapSprite && Y >= vm.Trait.H) { continue; }
		auto I = H * vm.Texture.W;

		for (auto W{ 0 }, X{ VX }; W < vm.Texture.W; ++W, ++X &= vm.Trait.Wb)
		{
			if (const auto sourceColorIdx{ vm.readMemoryI(I++) }; sourceColorIdx)
			{
				auto& collideCoord{ vm.collisionPalette.at_raw(Y, X) };
				auto& backbufCoord{ vm.backgroundBuffer.at_raw(Y, X) };

				if (collideCoord == vm.Texture.collision)
					[[unlikely]] { vm.mRegisterV[0xF] = 1; }

				collideCoord = sourceColorIdx;
				backbufCoord = blendPixel(
					vm.megaPalette[sourceColorIdx],
					backbufCoord, vm.Texture.alpha,
					blendAlgo
				);
			}
			if (!vm.Quirk.wrapSprite && X == vm.Trait.Wb) { break; }
		}
		if (!vm.Quirk.wrapSprite && Y == vm.Trait.Hb) { break; }
	}
}

void FunctionsForMegachip::chooseBlend(const usz N) noexcept {
	switch (N) {

		case Blend::LINEAR_DODGE:
			blendAlgo = [](const f32 src, const f32 dst) noexcept {
				return std::min(src + dst, 1.0f);
			};
			break;

		case Blend::MULTIPLY:
			blendAlgo = [](const f32 src, const f32 dst) noexcept {
				return src * dst;
			};
			break;

		default:
		case Blend::NORMAL:
			blendAlgo = [](const f32 src, const f32) noexcept {
				return src;
			};
			break;
	}
}
