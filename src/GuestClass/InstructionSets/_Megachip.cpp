/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>

#include "../../HostClass/BasicVideoSpec.hpp"

#include "Interface.hpp"
#include "../Guest.hpp"
#include "../Registers.hpp"
#include "../MemoryBanks.hpp"
#include "../DisplayColors.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForMegachip                  */
/*------------------------------------------------------------------*/

FunctionsForMegachip::FunctionsForMegachip(VM_Guest* parent)
	: vm{ parent }
{
	chooseBlend(Blend::NORMAL);
}

void FunctionsForMegachip::scrollUP(const std::int32_t N) {
	vm->Mem->foregroundBuffer.shift(-N, 0);
	blendToDisplay(
		vm->Mem->foregroundBuffer.data(),
		vm->Mem->backgroundBuffer.data(),
		vm->Plane.size
	);
}
void FunctionsForMegachip::scrollDN(const std::int32_t N) {
	vm->Mem->foregroundBuffer.shift(+N, 0);
	blendToDisplay(
		vm->Mem->foregroundBuffer.data(),
		vm->Mem->backgroundBuffer.data(),
		vm->Plane.size
	);
}
void FunctionsForMegachip::scrollLT(const std::int32_t N) {
	vm->Mem->foregroundBuffer.shift(0, -N);
	blendToDisplay(
		vm->Mem->foregroundBuffer.data(),
		vm->Mem->backgroundBuffer.data(),
		vm->Plane.size
	);
}
void FunctionsForMegachip::scrollRT(const std::int32_t N) {
	vm->Mem->foregroundBuffer.shift(0, +N);
	blendToDisplay(
		vm->Mem->foregroundBuffer.data(),
		vm->Mem->backgroundBuffer.data(),
		vm->Plane.size
	);
}

/*------------------------------------------------------------------*/

template <typename T>
void FunctionsForMegachip::blendToDisplay(
	const T* const src, const T* const dst,
	const std::size_t size
) {
	vm->Video->lockTexture();
	for (std::size_t idx{ 0 }; std::cmp_less(idx, size); ++idx) {
		vm->Video->pixels[idx] = blendPixel(src[idx], dst[idx]);
	}
	vm->Video->unlockTexture();
}

uint32_t FunctionsForMegachip::blendPixel(
	const std::uint32_t colorSrc,
	const std::uint32_t colorDst
) {
	static constexpr float minA{ 1.0f / 255.0f };
	src.A = (colorSrc >> 24) / 255.0f * vm->Trait.alpha;
	if (src.A < minA) [[unlikely]] return colorDst;

	src.R = ((colorSrc >> 16) & 0xFF) / 255.0f;
	src.G = ((colorSrc >>  8) & 0xFF) / 255.0f;
	src.B = ( colorSrc        & 0xFF) / 255.0f;

	dst.A = ( colorDst >> 24)         / 255.0f;
	dst.R = ((colorDst >> 16) & 0xFF) / 255.0f;
	dst.G = ((colorDst >>  8) & 0xFF) / 255.0f;
	dst.B = ( colorDst        & 0xFF) / 255.0f;

	return applyBlend(blendType);
}

uint32_t FunctionsForMegachip::applyBlend(
	float (*blend)(const float, const float)
) const {
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

	return 0xFF000000u
		| static_cast<std::uint8_t>(std::roundf(R * 255.0f)) << 16u
		| static_cast<std::uint8_t>(std::roundf(G * 255.0f)) <<  8u
		| static_cast<std::uint8_t>(std::roundf(B * 255.0f));
}

void FunctionsForMegachip::drawSprite(
	std::int32_t VX,
	std::int32_t VY,
	std::int32_t  N,
	std::uint32_t I
) {
	vm->Reg->V[0xF] = 0;

	if (I < 0xF0) [[unlikely]] { // font sprite rendering
		for (auto H{ 0 }; H < N; ++I, ++H, ++VY &= 0xFFu) {
			if (VY >= vm->Plane.H) [[unlikely]] continue;

			auto X{ VX };
			const auto srcIndex{ vm->mrw(I) }; // font byte

			for (auto W{ 7 }; W >= 0; --W, ++X &= 0xFFu) {
				if (!(srcIndex >> W & 0x1)) continue;

				auto& colorIdx{ vm->Mem->collisionPalette[VY][X] }; // DESTINATION pixel's collision megaPalette index
				auto& colorDst{ vm->Mem->backgroundBuffer[VY][X] }; // DESTINATION pixel's color

				if (colorIdx) [[unlikely]] {
					colorIdx = 0;
					colorDst = 0;
					vm->Reg->V[0xF] = 1;
				} else {
					colorIdx = 254;
					colorDst = vm->Color->hex[H];
				}
			}
		}
		return;
	}

	for (auto H{ 0 }; H < vm->Trait.H; ++H, ++VY &= 0xFFu) {
		if (VY >= vm->Plane.H) [[unlikely]] {
			I += vm->Trait.W; 
			continue;
		}
		auto X{ VX };
		for (auto W{ 0 }; W < vm->Trait.W; ++W, ++X &= 0xFFu) {
			const auto srcIndex{ vm->mrw(I++) }; // palette index from RAM 
			if (!srcIndex) continue;

			auto& colorIdx{ vm->Mem->collisionPalette[VY][X] }; // DESTINATION pixel's collision palette index
			auto& colorDst{ vm->Mem->backgroundBuffer[VY][X] }; // DESTINATION pixel's color

			if (!vm->Reg->V[0xF] && colorIdx == vm->Trait.collision) [[unlikely]]
				vm->Reg->V[0xF] = 1;

			colorIdx = srcIndex;
			colorDst = blendPixel(vm->Mem->megaPalette[srcIndex], colorDst);
		}
	}
}

void FunctionsForMegachip::chooseBlend(const std::size_t N) {
	switch (N) {

		case Blend::LINEAR_DODGE:
			blendType = [](const float src, const float dst) {
				return std::min(src + dst, 1.0f);
			};
			break;

		case Blend::MULTIPLY:
			blendType = [](const float src, const float dst) {
				return src * dst;
			};
			break;

		default:
		case Blend::NORMAL:
			blendType = [](const float src, const float) {
				return src;
			};
			break;
	}
}
