/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>

#include "Interface.hpp"
#include "../Guest.hpp"
#include "../Registers.hpp"
#include "../MemoryBanks.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForGigachip                  */
/*------------------------------------------------------------------*/

FunctionsForGigachip::FunctionsForGigachip(VM_Guest* parent) noexcept
	: vm{ parent }
{
	chooseBlend(Blend::NORMAL);
}

/*------------------------------------------------------------------*/

void FunctionsForGigachip::scrollUP(const std::int32_t N) {
	vm->Mem->foregroundBuffer.rotate(-N, 0);
}
void FunctionsForGigachip::scrollDN(const std::int32_t N) {
	vm->Mem->foregroundBuffer.rotate(+N, 0);
}
void FunctionsForGigachip::scrollLT(const std::int32_t N) {
	vm->Mem->foregroundBuffer.rotate(0, -N);
}
void FunctionsForGigachip::scrollRT(const std::int32_t N) {
	vm->Mem->foregroundBuffer.rotate(0, +N);
}

/*------------------------------------------------------------------*/

uint32_t FunctionsForGigachip::blendPixel(
	std::uint32_t  colorSrc,
	std::uint32_t& colorDst
) {
	static constexpr float minF{ 1.0f / 255.0f };

	src.A = (colorSrc >> 24) * minF * vm->Trait.alpha;
	if (src.A < minF) [[unlikely]] return colorDst; // pixel is fully transparent
	if (vm->Trait.invert) { colorSrc ^= 0x00FFFFFF; }
	src.R = (colorSrc >> 16 & 0xFF) * minF;
	src.G = (colorSrc >>  8 & 0xFF) * minF;
	src.B = (colorSrc       & 0xFF) * minF;

	dst.A = (colorDst >> 24       ) * minF;
	dst.R = (colorDst >> 16 & 0xFF) * minF;
	dst.G = (colorDst >>  8 & 0xFF) * minF;
	dst.B = (colorDst       & 0xFF) * minF;

	switch (vm->Trait.rgbmod) {
		case Trait::BRG:
			std::swap(src.R, src.G);
			std::swap(src.R, src.B);
			break;
		case Trait::GBR:
			std::swap(src.R, src.G);
			std::swap(src.G, src.B);
			break;
		case Trait::RBG:
			std::swap(src.G, src.B);
			break;
		case Trait::GRB:
			std::swap(src.R, src.G);
			break;
		case Trait::BGR:
			std::swap(src.R, src.B);
			break;
		case Trait::GRAY:
			src.R = src.G = src.B =
				src.R * 0.299f + src.G * 0.587f + src.B * 0.114f;
			break;
		case Trait::SEPIA: {
			const float R{ src.R * 0.393f + src.G * 0.769f + src.B * 0.198f },
						G{ src.R * 0.349f + src.G * 0.686f + src.B * 0.168f },
						B{ src.R * 0.272f + src.G * 0.534f + src.B * 0.131f };
			src.R = std::min(R, 1.0f);
			src.G = std::min(G, 1.0f);
			src.B = std::min(B, 1.0f);
		} break;
	}

	if (!blendType) {
		return static_cast<std::uint8_t>(std::round(src.A * 255.0f)) << 24
			 | static_cast<std::uint8_t>(std::round(src.R * 255.0f)) << 16
			 | static_cast<std::uint8_t>(std::round(src.G * 255.0f)) <<  8
			 | static_cast<std::uint8_t>(std::round(src.B * 255.0f));
	}
	else {
		return applyBlend(blendType);
	}
}

uint32_t FunctionsForGigachip::applyBlend(float (*blend)(const float, const float)) const {
	float A{ 1.0f };
	float R{ blend(src.R, dst.R) };
	float G{ blend(src.G, dst.G) };
	float B{ blend(src.B, dst.B) };

	if (src.A < 1.0f) {
		const float sW{ src.A / 1.0f };
		const float dW{ 1.0f - sW };

		A = dW * dst.A + src.A;
		R = dW * dst.R + sW * R;
		G = dW * dst.G + sW * G;
		B = dW * dst.B + sW * B;
	}

	return static_cast<std::uint8_t>(std::roundf(A * 255.0f)) << 24
		 | static_cast<std::uint8_t>(std::roundf(R * 255.0f)) << 16
		 | static_cast<std::uint8_t>(std::roundf(G * 255.0f)) <<  8
		 | static_cast<std::uint8_t>(std::roundf(B * 255.0f));
}

void FunctionsForGigachip::drawSprite(
	const std::int32_t VX,
	const std::int32_t VY,
	const std::int32_t  N,
	const std::uint32_t I
) {
	const auto currW{ vm->Trait.W }; auto tempW{ currW };
	const auto currH{ vm->Trait.H }; auto tempH{ currH };

	bool flipX{ vm->Trait.flip_X };
	bool flipY{ vm->Trait.flip_Y };

	vm->Trait.alpha = (N ^ 0xF) / 15.0f;

	if (vm->Trait.uneven) {
		std::swap(tempW, tempH);
		std::swap(flipX, flipY);
	}

	auto memY{ 0 }, memX{ 0 }; // position vars for RAM access
	auto collideHits{ 0 };
	
	for (auto H{ 0 }, Y{ VY }; H < tempH; ++H, ++Y %= vm->Plane.H) {
		for (auto W{ 0 }, X{ VX }; W < tempW; ++W, ++X &= 0xFF) {

			if (vm->Trait.rotate) {
				memX = H;
				memY = tempW - W - 1;
			} else {
				memX = W;
				memY = H;
			}

			if (flipX) { memX = currW - memX - 1; }
			if (flipY) { memY = currH - memY - 1; }

			const auto srcColorIndex{ vm->mrw(I + (memY * currW) + memX) };
			if (!srcColorIndex) continue;

			auto& collideCoord{ vm->Mem->collisionPalette.at_raw(Y, X) };
			auto& backbufCoord{ vm->Mem->backgroundBuffer.at_raw(Y, X) };
			
			if (collideCoord == vm->Trait.collision)
				[[unlikely]] { ++collideHits; }
			
			collideCoord = srcColorIndex;
			if (vm->Trait.nodraw) continue;
			backbufCoord = blendPixel(
				vm->Mem->megaPalette[srcColorIndex],
				backbufCoord
			);
		}
	}

	vm->Reg->V[0xF] = collideHits != 0;
}

void FunctionsForGigachip::chooseBlend(
	const std::size_t N
) {
	switch (N) {

		case Blend::NORMAL:
			blendType = [](const float src, const float) {
				return src;
			};
			return;

		/*------------------------ LIGHTENING MODES ------------------------*/

		case Blend::LIGHTEN_ONLY:
			blendType = [](const float src, const float dst) {
				return std::max(src, dst);
			};
			return;

		case Blend::SCREEN:
			blendType = [](const float src, const float dst) {
				return 1.0f - (1.0f - src) * (1.0f - dst);
			};
			return;

		case Blend::COLOR_DODGE:
			blendType = [](const float src, const float dst) {
				return std::min(dst / (1.0f - src), 1.0f);
			};
			return;

		case Blend::LINEAR_DODGE:
			blendType = [](const float src, const float dst) {
				return std::min(src + dst, 1.0f);
			};
			return;

		/*------------------------ DARKENING MODES -------------------------*/

		case Blend::DARKEN_ONLY:
			blendType = [](const float src, const float dst) {
				return std::min(src, dst);
			};
			return;

		case Blend::MULTIPLY:
			blendType = [](const float src, const float dst) {
				return src * dst;
			};
			return;

		case Blend::COLOR_BURN:
			blendType = [](const float src, const float dst) {
				if (!src) return 0.0f; // handle divide-by-zero
				return std::max(1.0f - (1.0f - dst) / src, 0.0f);
			};
			return;

		case Blend::LINEAR_BURN:
			blendType = [](const float src, const float dst) {
				return std::max(src + dst - 1.0f, 0.0f);
			};
			return;

		/*-------------------------- OTHER MODES ---------------------------*/

		case Blend::AVERAGE:
			blendType = [](const float src, const float dst) {
				return (src + dst) / 2.0f;
			};
			return;

		case Blend::DIFFERENCE:
			blendType = [](const float src, const float dst) {
				return std::abs(src - dst);
			};
			return;

		case Blend::NEGATION:
			blendType = [](const float src, const float dst) {
				return 1.0f - std::abs(1.0f - src - dst);
			}; return;

		case Blend::OVERLAY:
			blendType = [](const float src, const float dst) {
				if (src < 0.5f) return 2.0f * dst * src;
				return 1.0f - 2.0f * (1.0f - dst) * (1.0f - src);
			};
			return;

		case Blend::REFLECT:
			blendType = [](const float src, const float dst) {
				if (src == 1.0f) return 1.0f; // handle divide-by-zero
				return std::min(dst * dst / (1.0f - src), 1.0f);
			};
			return;

		case Blend::GLOW:
			blendType = [](const float src, const float dst) {
				if (dst == 1.0f) return 1.0f; // handle divide-by-zero
				return std::min(src * src / (1.0f - dst), 1.0f);
			};
			return;

		case Blend::OVERWRITE:
			blendType = nullptr;
			return;
	}
}
