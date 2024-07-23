/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>

#include "../../HostClass/BasicVideoSpec.hpp"

#include "Interface.hpp"
#include "../Guest.hpp"
#include "../DisplayTraits.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForMegachip                  */
/*------------------------------------------------------------------*/

FunctionsForMegachip::FunctionsForMegachip(VM_Guest& parent) noexcept
	: vm{ parent }
{
	chooseBlend(Blend::NORMAL);
}

void FunctionsForMegachip::scrollUP(const s32 N) {
	vm.foregroundBuffer.shift(-N, 0);
	blendToDisplay(
		vm.foregroundBuffer.data(),
		vm.backgroundBuffer.data(),
		vm.Display->Trait.S
	);
}
void FunctionsForMegachip::scrollDN(const s32 N) {
	vm.foregroundBuffer.shift(+N, 0);
	blendToDisplay(
		vm.foregroundBuffer.data(),
		vm.backgroundBuffer.data(),
		vm.Display->Trait.S
	);
}
void FunctionsForMegachip::scrollLT(const s32 N) {
	vm.foregroundBuffer.shift(0, -N);
	blendToDisplay(
		vm.foregroundBuffer.data(),
		vm.backgroundBuffer.data(),
		vm.Display->Trait.S
	);
}
void FunctionsForMegachip::scrollRT(const s32 N) {
	vm.foregroundBuffer.shift(0, +N);
	blendToDisplay(
		vm.foregroundBuffer.data(),
		vm.backgroundBuffer.data(),
		vm.Display->Trait.S
	);
}

/*------------------------------------------------------------------*/

template <typename T>
void FunctionsForMegachip::blendToDisplay(
	const T* const src, const T* const dst,
	const usz size
) {
	auto* pixels{ vm.BVS.lockTexture() };
	for (usz idx{ 0 }; idx < size; ++idx) {
		pixels[idx] = blendPixel(src[idx], dst[idx]);
	}
	vm.BVS.unlockTexture();
}

uint32_t FunctionsForMegachip::blendPixel(
	const u32 colorSrc,
	const u32 colorDst
)  noexcept {
	static constexpr float minF{ 1.0f / 255.0f };

	src.A = (colorSrc >> 24) * minF * vm.Display->Tex.alpha;
	if (src.A < minF) [[unlikely]] { return colorDst; }
	src.R = ((colorSrc >> 16) & 0xFF) * minF;
	src.G = ((colorSrc >>  8) & 0xFF) * minF;
	src.B = ( colorSrc        & 0xFF) * minF;

	dst.A = ( colorDst >> 24)         * minF;
	dst.R = ((colorDst >> 16) & 0xFF) * minF;
	dst.G = ((colorDst >>  8) & 0xFF) * minF;
	dst.B = ( colorDst        & 0xFF) * minF;

	return applyBlend(blendType);
}

uint32_t FunctionsForMegachip::applyBlend(
	float (*blend)(const float, const float)
) const noexcept {
	float R{ blend(src.R, dst.R) };
	float G{ blend(src.G, dst.G) };
	float B{ blend(src.B, dst.B) };

	if (src.A < 1.0f) {
		const float sW{ src.A / 1.0f };
		const float dW{ 1.0f - sW };

		R = dW * dst.R + sW * R;
		G = dW * dst.G + sW * G;
		B = dW * dst.B + sW * B;
	}

	return 0xFF000000
		| static_cast<std::uint8_t>(std::roundf(R * 255.0f)) << 16
		| static_cast<std::uint8_t>(std::roundf(G * 255.0f)) <<  8
		| static_cast<std::uint8_t>(std::roundf(B * 255.0f));
}

void FunctionsForMegachip::drawSprite(
	s32 _X, s32 _Y, s32 FR
) {
	s32 VX{ vm.mRegisterV[_X] };
	s32 VY{ vm.mRegisterV[_Y] };

	vm.mRegisterV[0xF] = 0;
	if (!vm.Quirk.wrapSprite && VY >= vm.Display->Trait.H) { return; }
	if (vm.mRegisterI >= 0xF0) [[likely]] { goto paintTexture; }

	for (auto H{ 0 }, Y{ VY }; H < FR; ++H, ++Y &= vm.Display->Trait.Wb)
	{
		if (vm.Quirk.wrapSprite && Y >= vm.Display->Trait.H) { continue; }
		const auto bytePixel{ vm.readMemoryI(H) };

		for (auto W{ 7 }, X{ VX }; W >= 0; --W, ++X &= vm.Display->Trait.Wb)
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
					backbufCoord = vm.Display->Color.hex[H];
				}
			}
			if (!vm.Quirk.wrapSprite && X == vm.Display->Trait.Wb) { break; }
		}
		if (!vm.Quirk.wrapSprite && Y == vm.Display->Trait.Hb) { break; }
	}
	return;

paintTexture:
	for (auto H{ 0 }, Y{ VY }; H < vm.Display->Tex.H; ++H, ++Y &= vm.Display->Trait.Wb)
	{
		if (vm.Quirk.wrapSprite && Y >= vm.Display->Trait.H) { continue; }
		auto I = H * vm.Display->Tex.W;

		for (auto W{ 0 }, X{ VX }; W < vm.Display->Tex.W; ++W, ++X &= vm.Display->Trait.Wb)
		{
			if (const auto sourceColorIdx{ vm.readMemoryI(I++) }; sourceColorIdx)
			{
				auto& collideCoord{ vm.collisionPalette.at_raw(Y, X) };
				auto& backbufCoord{ vm.backgroundBuffer.at_raw(Y, X) };

				if (collideCoord == vm.Display->Tex.collision)
					[[unlikely]] { vm.mRegisterV[0xF] = 1; }

				collideCoord = sourceColorIdx;
				backbufCoord = blendPixel(
					vm.megaPalette[sourceColorIdx],
					backbufCoord
				);
			}
			if (!vm.Quirk.wrapSprite && X == vm.Display->Trait.Wb) { break; }
		}
		if (!vm.Quirk.wrapSprite && Y == vm.Display->Trait.Hb) { break; }
	}
}

void FunctionsForMegachip::chooseBlend(const usz N) noexcept {
	switch (N) {

		case Blend::LINEAR_DODGE:
			blendType = [](const float src, const float dst) noexcept {
				return std::min(src + dst, 1.0f);
			};
			break;

		case Blend::MULTIPLY:
			blendType = [](const float src, const float dst) noexcept {
				return src * dst;
			};
			break;

		default:
		case Blend::NORMAL:
			blendType = [](const float src, const float) noexcept {
				return src;
			};
			break;
	}
}
