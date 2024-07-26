/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>

#include "Interface.hpp"
#include "../Guest.hpp"

/*------------------------------------------------------------------*/
/*  class  FncSetInterface -> FunctionsForGigachip                  */
/*------------------------------------------------------------------*/

FunctionsForGigachip::FunctionsForGigachip(VM_Guest& parent) noexcept
	: vm{ parent }
{
	chooseBlend(Blend::NORMAL);
}

/*------------------------------------------------------------------*/

void FunctionsForGigachip::scrollUP(const s32 N) {
	vm.foregroundBuffer.rotate(-N, 0);
}
void FunctionsForGigachip::scrollDN(const s32 N) {
	vm.foregroundBuffer.rotate(+N, 0);
}
void FunctionsForGigachip::scrollLT(const s32 N) {
	vm.foregroundBuffer.rotate(0, -N);
}
void FunctionsForGigachip::scrollRT(const s32 N) {
	vm.foregroundBuffer.rotate(0, +N);
}

/*------------------------------------------------------------------*/

u32 FunctionsForGigachip::blendPixel(
	u32  colorSrc,
	u32& colorDst
) {
	static constexpr float minF{ 1.0f / 255.0f };

	src.A = (colorSrc >> 24) * minF * vm.Texture.alpha;
	if (src.A < minF) [[unlikely]] { return colorDst; }
	if (vm.Texture.invert) { colorSrc ^= 0x00FFFFFF; }
	src.R = (colorSrc >> 16 & 0xFF) * minF;
	src.G = (colorSrc >>  8 & 0xFF) * minF;
	src.B = (colorSrc       & 0xFF) * minF;

	dst.A = (colorDst >> 24       ) * minF;
	dst.R = (colorDst >> 16 & 0xFF) * minF;
	dst.G = (colorDst >>  8 & 0xFF) * minF;
	dst.B = (colorDst       & 0xFF) * minF;

	switch (vm.Texture.rgbmod) {
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
		return static_cast<u8>(std::round(src.A * 255.0f)) << 24
			 | static_cast<u8>(std::round(src.R * 255.0f)) << 16
			 | static_cast<u8>(std::round(src.G * 255.0f)) <<  8
			 | static_cast<u8>(std::round(src.B * 255.0f));
	}
	else {
		return applyBlend(blendType);
	}
}

u32 FunctionsForGigachip::applyBlend(float (*blend)(const float, const float)) const {
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

	return static_cast<u8>(std::roundf(A * 255.0f)) << 24
		 | static_cast<u8>(std::roundf(R * 255.0f)) << 16
		 | static_cast<u8>(std::roundf(G * 255.0f)) <<  8
		 | static_cast<u8>(std::roundf(B * 255.0f));
}

void FunctionsForGigachip::drawSprite(
	s32 _X, s32 _Y, s32 N
) {
	s32 VX{ vm.mRegisterV[_X] };
	s32 VY{ vm.mRegisterV[_Y] };
	vm.mRegisterV[0xF] = 0;

	const auto currW{ vm.Texture.W }; auto tempW{ currW };
	const auto currH{ vm.Texture.H }; auto tempH{ currH };

	bool flipX{ vm.Texture.flip_X };
	bool flipY{ vm.Texture.flip_Y };

	vm.Texture.alpha = (N ^ 0xF) / 15.0f;

	if (vm.Texture.uneven) {
		std::swap(tempW, tempH);
		std::swap(flipX, flipY);
	}

	auto memY{ 0 }, memX{ 0 }; // position vars for RAM access

	for (auto H{ 0 }, Y{ VY }; H < tempH; ++H, ++Y %= vm.Trait.H) {
		for (auto W{ 0 }, X{ VX }; W < tempW; ++W, ++X &= vm.Trait.Wb) {

			if (vm.Texture.rotate) {
				memX = H; memY = tempW - W - 1;
			} else {
				memX = W; memY = H;
			}

			if (flipX) { memX = currW - memX - 1; }
			if (flipY) { memY = currH - memY - 1; }

			const auto sourceColorIdx{ vm.readMemoryI((memY * currW) + memX) };
			if (sourceColorIdx) {
				auto& collideCoord{ vm.collisionPalette.at_raw(Y, X) };
				auto& backbufCoord{ vm.backgroundBuffer.at_raw(Y, X) };

				if (collideCoord == vm.Texture.collision)
					[[unlikely]] { vm.mRegisterV[0xF] = 1; }

				collideCoord = sourceColorIdx;
				if (!vm.Texture.nodraw) {
					backbufCoord = blendPixel(
						vm.megaPalette[sourceColorIdx],
						backbufCoord
					);
				}
			}
			if (!vm.Quirk.wrapSprite && X == vm.Trait.Wb) { break; }
		}
		if (!vm.Quirk.wrapSprite && Y == vm.Trait.Hb) { break; }
	}
}

void FunctionsForGigachip::chooseBlend(
	const usz N
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
