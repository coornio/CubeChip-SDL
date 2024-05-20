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

FunctionsForGigachip::FunctionsForGigachip(VM_Guest* parent)
	: vm{ parent }
{
	chooseBlend(Blend::NORMAL);
}

/*------------------------------------------------------------------*/

void FunctionsForGigachip::scrollUP(const std::int32_t N) {
	vm->isDisplayReady(true);
	vm->Mem->display.rotate(-N, 0);
}
void FunctionsForGigachip::scrollDN(const std::int32_t N) {
	vm->isDisplayReady(true);
	vm->Mem->display.rotate(+N, 0);
}
void FunctionsForGigachip::scrollLT(const std::int32_t N) {
	vm->isDisplayReady(true);
	vm->Mem->display.rotate(0, -N);
}
void FunctionsForGigachip::scrollRT(const std::int32_t N) {
	vm->isDisplayReady(true);
	vm->Mem->display.rotate(0, +N);
}

/*------------------------------------------------------------------*/

uint32_t FunctionsForGigachip::blendPixel(
	std::uint32_t  colorSrc,
	std::uint32_t& colorDst
) {
	static constexpr float minA{ 1.0f / 255.0f };
	src.A = (colorSrc >> 24) / 255.0f * vm->Trait.alpha;
	if (src.A < minA) [[unlikely]] return colorDst; // pixel is fully transparent

	if (vm->Trait.invert) colorSrc ^= 0x00'FF'FF'FF;

	src.R = (colorSrc >> 16 & 0xFF) / 255.0f;
	src.G = (colorSrc >>  8 & 0xFF) / 255.0f;
	src.B = (colorSrc & 0xFF) / 255.0f;

	dst.A = (colorDst >> 24       ) / 255.0f;
	dst.R = (colorDst >> 16 & 0xFF) / 255.0f;
	dst.G = (colorDst >>  8 & 0xFF) / 255.0f;
	dst.B = (colorDst & 0xFF) / 255.0f;

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
	std::int32_t VX,
	std::int32_t VY,
	std::int32_t  N,
	std::uint32_t I
) {
	vm->Reg->V[0xF] = 0;
	
	const auto oW{ vm->Trait.W }; auto tW{ oW };
	const auto oH{ vm->Trait.H }; auto tH{ oH };

	bool fX{ vm->Trait.flip_X };
	bool fY{ vm->Trait.flip_Y };

	vm->Trait.alpha = (N ^ 0xF) / 15.0f;

	if (vm->Trait.uneven) {
		std::swap(tW, tH);
		std::swap(fX, fY);
	}

	std::size_t memY{}, memX{}; // position vars for RAM access
	
	for (auto H{ 0 }; H < tH; ++H, ++VY %= vm->Plane.H) {
		for (auto W{ 0 }; W < tW; ++W, ++VX &= 0xFFu) {

			if (vm->Trait.rotate) {
				memX = H;
				memY = tW - W - 1;
			}				
			else {
				memX = W;
				memY = H;
			}

			if (fX) { memX = oW - memX - 1; }
			if (fY) { memY = oH - memY - 1; }

			const auto srcIndex{ vm->mrw(I + (memY * oW) + memX) }; // palette index from RAM 
			if (!srcIndex) continue;

			auto& colorDst{ vm->Mem->bufColorMC[VY][VX] }; // DESTINATION pixel's destination color
			auto& colorIdx{ vm->Mem->bufPalette[VY][VX] }; // DESTINATION pixel's color/collision palette index
			
			if (!vm->Reg->V[0xF] && colorIdx == vm->Trait.collision) [[unlikely]] {
				vm->Reg->V[0xF] = 1;
			}
				
			colorIdx = srcIndex;

			if (vm->Trait.nodraw) continue;

			colorDst = blendPixel(vm->Mem->palette[srcIndex], colorDst);
		}
		VX -= vm->Trait.W;
	}
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
