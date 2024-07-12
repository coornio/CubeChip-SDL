/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>

#include "../../HostClass/BasicVideoSpec.hpp"

#include "Interface.hpp"
#include "../Guest.hpp"
#include "../MemoryBanks.hpp"
#include "../DisplayTraits.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForMegachip                  */
/*------------------------------------------------------------------*/

FunctionsForMegachip::FunctionsForMegachip(VM_Guest* parent) noexcept
	: vm{ parent }
{
	chooseBlend(Blend::NORMAL);
}

void FunctionsForMegachip::scrollUP(const std::int32_t N) {
	vm->Mem->foregroundBuffer.shift(-N, 0);
	blendToDisplay(
		vm->Mem->foregroundBuffer.data(),
		vm->Mem->backgroundBuffer.data(),
		vm->Display->Trait.S
	);
}
void FunctionsForMegachip::scrollDN(const std::int32_t N) {
	vm->Mem->foregroundBuffer.shift(+N, 0);
	blendToDisplay(
		vm->Mem->foregroundBuffer.data(),
		vm->Mem->backgroundBuffer.data(),
		vm->Display->Trait.S
	);
}
void FunctionsForMegachip::scrollLT(const std::int32_t N) {
	vm->Mem->foregroundBuffer.shift(0, -N);
	blendToDisplay(
		vm->Mem->foregroundBuffer.data(),
		vm->Mem->backgroundBuffer.data(),
		vm->Display->Trait.S
	);
}
void FunctionsForMegachip::scrollRT(const std::int32_t N) {
	vm->Mem->foregroundBuffer.shift(0, +N);
	blendToDisplay(
		vm->Mem->foregroundBuffer.data(),
		vm->Mem->backgroundBuffer.data(),
		vm->Display->Trait.S
	);
}

/*------------------------------------------------------------------*/

template <typename T>
void FunctionsForMegachip::blendToDisplay(
	const T* const src, const T* const dst,
	const std::size_t size
) {
	auto* pixels{ vm->BVS->lockTexture() };
	for (std::size_t idx{ 0 }; idx < size; ++idx) {
		pixels[idx] = blendPixel(src[idx], dst[idx]);
	}
	vm->BVS->unlockTexture();
}

uint32_t FunctionsForMegachip::blendPixel(
	const std::uint32_t colorSrc,
	const std::uint32_t colorDst
)  noexcept {
	static constexpr float minF{ 1.0f / 255.0f };

	src.A = (colorSrc >> 24) * minF * vm->Display->Tex.alpha;
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
	MemoryBanks* Mem, DisplayTraits* Display,
	std::int32_t _X, std::int32_t _Y, std::int32_t FR
) {
	std::int32_t VX{ Mem->vRegister[_X] };
	std::int32_t VY{ Mem->vRegister[_Y] };

	Mem->vRegister[0xF] = 0;
	if (!vm->Quirk.wrapSprite && VY >= Display->Trait.H) { return; }
	if (vm->Mem->index_get() >= 0xF0) [[likely]] { goto paintTexture; }

	for (auto H{ 0 }, Y{ VY }; H < FR; ++H, ++Y &= Display->Trait.Wb)
	{
		if (vm->Quirk.wrapSprite && Y >= Display->Trait.H) { continue; }
		const auto bytePixel{ Mem->read_idx(H) };

		for (auto W{ 7 }, X{ VX }; W >= 0; --W, ++X &= Display->Trait.Wb)
		{
			if (bytePixel >> W & 0x1)
			{
				auto& collideCoord{ Mem->collisionPalette.at_raw(Y, X) };
				auto& backbufCoord{ Mem->backgroundBuffer.at_raw(Y, X) };

				if (collideCoord) [[unlikely]] {
					collideCoord = 0;
					backbufCoord = 0;
					Mem->vRegister[0xF] = 1;
				} else {
					collideCoord = 254;
					backbufCoord = Display->Color.hex[H];
				}
			}
			if (!vm->Quirk.wrapSprite && X == Display->Trait.Wb) { break; }
		}
		if (!vm->Quirk.wrapSprite && Y == Display->Trait.Hb) { break; }
	}
	return;

paintTexture:
	for (auto H{ 0 }, Y{ VY }; H < Display->Tex.H; ++H, ++Y &= Display->Trait.Wb)
	{
		if (vm->Quirk.wrapSprite && Y >= Display->Trait.H) { continue; }
		auto I = H * Display->Tex.W;

		for (auto W{ 0 }, X{ VX }; W < Display->Tex.W; ++W, ++X &= Display->Trait.Wb)
		{
			if (const auto sourceColorIdx{ Mem->read_idx(I++) }; sourceColorIdx)
			{
				auto& collideCoord{ Mem->collisionPalette.at_raw(Y, X) };
				auto& backbufCoord{ Mem->backgroundBuffer.at_raw(Y, X) };

				if (collideCoord == Display->Tex.collision)
					[[unlikely]] { Mem->vRegister[0xF] = 1; }

				collideCoord = sourceColorIdx;
				backbufCoord = blendPixel(
					Mem->megaPalette[sourceColorIdx],
					backbufCoord
				);
			}
			if (!vm->Quirk.wrapSprite && X == Display->Trait.Wb) { break; }
		}
		if (!vm->Quirk.wrapSprite && Y == Display->Trait.Hb) { break; }
	}
}

void FunctionsForMegachip::chooseBlend(const std::size_t N) noexcept {
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
