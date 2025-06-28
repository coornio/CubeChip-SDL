/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cmath>
#include <cstdint>
#include <numbers>
#include <algorithm>

#ifndef __cpp_constexpr
	#define __cpp_constexpr 0
#endif

#if __cpp_constexpr >= 202211L
	#define CONSTEXPR_MATH constexpr
#else
	#define CONSTEXPR_MATH
#endif

/*==================================================================*/

class WaveForms {
	using Millis = std::uint32_t;
	using Type_B = std::uint8_t;
	using Type_F = float;

	static constexpr Type_F calc_period(Millis p, Millis t) noexcept
		{ return p ? Type_F(t % p) / p : 0.0f; }

public:
	class WavePoint {
		Type_F mPointValue{};

	public:
		constexpr WavePoint() noexcept = default;
		constexpr WavePoint(Type_F value)
			: mPointValue{ std::clamp(value, -1.0f, 1.0f) }
		{}

		template <std::integral T>
		constexpr WavePoint(T value) noexcept
			: mPointValue{ std::clamp(value, T(0), T(255)) / 127.5f - 1.0f }
		{}

		// normalize WavePoint value to a 0..1 range and return
		constexpr Type_F normalize() const noexcept
			{ return static_cast<Type_F>(mPointValue * 0.5f + 0.5f); }

		// normalize WavePoint value to a 0..255 range and return
		constexpr Type_B byte_cast() const noexcept
			{ return static_cast<Type_B>(mPointValue * 127.5f + 127.5f); }

		constexpr operator Type_F() const noexcept { return mPointValue; }
		constexpr operator Type_B() const noexcept { return byte_cast(); }
	};

	static CONSTEXPR_MATH WavePoint cos(Millis p, Millis t) noexcept
		{ return Type_F(std::cos(std::numbers::pi * 2 * calc_period(p, t))); }

	static CONSTEXPR_MATH WavePoint sin(Millis p, Millis t) noexcept
		{ return Type_F(std::sin(std::numbers::pi * 2 * calc_period(p, t))); }
	
	static constexpr WavePoint square(Millis p, Millis t) noexcept
		{ return calc_period(p, t) < 0.5f ? 1.0f : -1.0f; }

	static constexpr WavePoint sawtooth(Millis p, Millis t) noexcept
		{ return 2.0f * calc_period(p, t) - 1.0f; }

	static CONSTEXPR_MATH WavePoint triangle(Millis p, Millis t) noexcept {
		const auto phase{ calc_period(p, t) };
		return 2.0f * std::abs(2.0f * (phase - std::floor(phase + 0.5f))) - 1.0f;
	}
};

namespace EzMaths {
	using Type_B = std::uint8_t;
	using Weight = WaveForms::WavePoint;

	// simple constexpr-enabled fmod, internally allows s32-width division
	template <std::floating_point T>
	inline constexpr T fmod(T x, T y) noexcept {
		return y ? x - y * static_cast<int>(x / y) : x;
	}

	template <std::floating_point T>
	inline constexpr T round(T x) noexcept {
		return x >= T(0)
			? T(static_cast<long long>(x + T(0.5)))
			: T(static_cast<long long>(x - T(0.5)));
	}

	inline constexpr Type_B fixedMul8(Type_B x, Type_B y) noexcept {
		return Type_B(((x * (y | y << 8)) + 0x8080u) >> 16);
	}

	inline constexpr Type_B fixedLerp8(Type_B x, Type_B y, Weight w) noexcept {
		return Type_B(fixedMul8(x, 255u - w.byte_cast()) + fixedMul8(y, w.byte_cast()));
	}

	template <std::integral T>
	inline constexpr T fixedLerpN(T x, T y, Weight w, T full_hue, T half_hue) noexcept {
		const auto shortest{ (y - x + half_hue) % full_hue - half_hue };
		return T((x + T(shortest * w.normalize()) + full_hue) % full_hue);
	}
}

/*==================================================================*/

struct RGBA;
struct HSV;
struct OKLAB;
struct OKLCH;

inline constexpr      HSV   to_HSV  (RGBA  in) noexcept;
inline constexpr      RGBA  to_RGBA (HSV   in) noexcept;
inline CONSTEXPR_MATH OKLAB to_OKLAB(RGBA  in) noexcept;
inline CONSTEXPR_MATH OKLAB to_OKLAB(OKLCH in) noexcept;
inline CONSTEXPR_MATH OKLCH to_OKLCH(OKLAB in) noexcept;
inline CONSTEXPR_MATH OKLCH to_OKLCH(RGBA  in) noexcept;
inline CONSTEXPR_MATH RGBA  to_RGBA (OKLAB in) noexcept;
inline CONSTEXPR_MATH RGBA  to_RGBA (OKLCH in) noexcept;

/*==================================================================*/

struct alignas(4) RGBA {
	using type_C = std::uint8_t;
	using Packed = std::uint32_t;
	using Weight = WaveForms::WavePoint;

	type_C R{}, G{}, B{}, A{};

	constexpr RGBA() noexcept = default;
	constexpr RGBA(Packed color) noexcept
		: R{ type_C(color >> 24) }
		, G{ type_C(color >> 16) }
		, B{ type_C(color >>  8) }
		, A{ type_C(color >>  0) }
	{}
	constexpr RGBA(type_C R, type_C G, type_C B, type_C A = 0xFF) noexcept
		: R{ R }, G{ G }, B{ B }, A{ A }
	{}

	constexpr Packed RGB_()     const noexcept { return R << 24 | G << 16 | B << 8 | 0; }
	constexpr Packed RBG_()     const noexcept { return R << 24 | B << 16 | G << 8 | 0; }
	constexpr Packed GRB_()     const noexcept { return G << 24 | R << 16 | B << 8 | 0; }
	constexpr Packed GBR_()     const noexcept { return G << 24 | B << 16 | R << 8 | 0; }
	constexpr Packed BRG_()     const noexcept { return B << 24 | R << 16 | G << 8 | 0; }
	constexpr Packed BGR_()     const noexcept { return B << 24 | G << 16 | R << 8 | 0; }

	constexpr Packed RBGA()     const noexcept { return R << 24 | B << 16 | G << 8 | A; }
	constexpr Packed GRBA()     const noexcept { return G << 24 | R << 16 | B << 8 | A; }
	constexpr Packed GBRA()     const noexcept { return G << 24 | B << 16 | R << 8 | A; }
	constexpr Packed BRGA()     const noexcept { return B << 24 | R << 16 | G << 8 | A; }
	constexpr Packed BGRA()     const noexcept { return B << 24 | G << 16 | R << 8 | A; }
	constexpr operator Packed() const noexcept { return R << 24 | G << 16 | B << 8 | A; }

	static constexpr RGBA lerp(RGBA x, RGBA y, Weight w) noexcept {
		return RGBA(
			EzMaths::fixedLerp8(x.R, y.R, w),
			EzMaths::fixedLerp8(x.G, y.G, w),
			EzMaths::fixedLerp8(x.B, y.B, w),
			EzMaths::fixedLerp8(x.A, y.A, w));
	}
};

/*==================================================================*/

struct alignas(4) HSV {
	using type_H = std::int16_t;
	using type_S = std::uint8_t;
	using type_V = type_S;
	using Packed = std::uint32_t;
	using Weight = WaveForms::WavePoint;

	static constexpr auto full_hue{ type_H(0x600u) };
	static constexpr auto half_hue{ type_H(full_hue >> 1) };

	type_H H{};
	type_S S{};
	type_V V{};

	constexpr HSV() noexcept = default;
	constexpr HSV(Packed color) noexcept
		: H{ type_H(color >> 16) }
		, S{ type_S(color >>  8) }
		, V{ type_V(color >>  0) }
	{}
	constexpr HSV(type_H H, type_S S, type_V V) noexcept
		: H{ H }, S{ S }, V{ V }
	{}

	constexpr operator Packed() const noexcept { return Packed(H) << 16 | S << 8 | V; }

	static constexpr HSV lerp(HSV x, HSV y, Weight w) noexcept {
		return HSV(
			EzMaths::fixedLerpN(x.H, y.H, w, full_hue, half_hue),
			EzMaths::fixedLerp8(x.S, y.S, w),
			EzMaths::fixedLerp8(x.V, y.V, w));
	}
};

/*==================================================================*/

struct OKLAB {
	using type_F = float;
	using Weight = WaveForms::WavePoint;

	type_F L{}, A{}, B{};

	constexpr OKLAB() noexcept = default;
	constexpr OKLAB(type_F L, type_F A, type_F B) noexcept
		: L{ L }, A{ A }, B{ B }
	{}

	static CONSTEXPR_MATH type_F gamma_def(type_F x) noexcept {
		return x <= 0.0404500f ? x / 12.92f : std::pow((x + 0.055f) / 1.055f, 2.4f);
		//return std::pow(x, 2.2); // quick path
	}
	static CONSTEXPR_MATH type_F gamma_inv(type_F x) noexcept {
		return x <= 0.0031308f ? x * 12.92f : 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;
		//return std::pow(x, 1.0f / 2.2f); // quick path
	}

	static constexpr OKLAB lerp(OKLAB x, OKLAB y, Weight w) noexcept {
		return OKLAB(
			std::lerp(x.L, y.L, w.normalize()),
			std::lerp(x.A, y.A, w.normalize()),
			std::lerp(x.B, y.B, w.normalize()));
	}

	static CONSTEXPR_MATH RGBA lerp(RGBA x, RGBA y, Weight w) noexcept {
		return ::to_RGBA(lerp(::to_OKLAB(x), ::to_OKLAB(y), w));
	}
};

struct OKLCH {
	using type_F = OKLAB::type_F;
	using Weight = OKLAB::Weight;

	type_F L{}, C{}, H{};

	constexpr OKLCH() noexcept = default;
	constexpr OKLCH(type_F L, type_F C, type_F H) noexcept
		: L{ L }, C{ C }, H{ H }
	{}

	static constexpr OKLCH lerp(OKLCH x, OKLCH y, Weight w) noexcept {
		const auto delta{ EzMaths::fmod(y.H - x.H + \
			std::numbers::pi * 3, std::numbers::pi * 2) - std::numbers::pi };
		return OKLCH(
			std::lerp(x.L, y.L, w.normalize()),
			std::lerp(x.C, y.C, w.normalize()),
			type_F(y.H + delta * w.normalize()));
	}

	static CONSTEXPR_MATH OKLAB lerp(OKLAB x, OKLAB y, Weight w) noexcept {
		return ::to_OKLAB(lerp(::to_OKLCH(x), ::to_OKLCH(y), w));
	}

	static CONSTEXPR_MATH RGBA lerp(RGBA x, RGBA y, Weight w) noexcept {
		return ::to_RGBA(lerp(::to_OKLCH(x), ::to_OKLCH(y), w));
	}
};

/*==================================================================*/

inline constexpr HSV to_HSV(RGBA in) noexcept {
	const auto maxV{ std::max({ in.R, in.G, in.B }) };
	const auto minV{ std::min({ in.R, in.G, in.B }) };
	const auto diff{ maxV - minV };
	
	if (diff == 0)
		{ return HSV(0, 0, maxV); }

	auto hueV{ 0 };

	/**/ if (maxV == in.R)
		{ hueV = 0x000 + ((in.G - in.B) * 0x100 / diff); }
	else if (maxV == in.G)
		{ hueV = 0x200 + ((in.B - in.R) * 0x100 / diff); }
	else if (maxV == in.B)
		{ hueV = 0x400 + ((in.R - in.G) * 0x100 / diff); }

	return HSV(HSV::type_H((hueV + HSV::full_hue) % HSV::full_hue), \
		HSV::type_S((diff * 0xFF + (maxV >> 1)) / maxV), HSV::type_V(maxV));
}

inline constexpr RGBA to_RGBA(HSV in) noexcept {
	if (in.S == 0x00) [[unlikely]]
		{ return RGBA(in.V, in.V, in.V); }

	const auto hueV{ in.H & 0xFF };

	const auto valP{ RGBA::type_C((in.V * (0x00FF - in.S)                  + 0x007F) / 0x00FF) };
	const auto valQ{ RGBA::type_C((in.V * (0xFF00 - in.S *          hueV)  + 0x7FFF) / 0xFF00) };
	const auto valT{ RGBA::type_C((in.V * (0xFF00 - in.S * (0x100 - hueV)) + 0x7FFF) / 0xFF00) };

	switch (in.H >> 8) {
		case 0: return RGBA(in.V, valT, valP);
		case 1: return RGBA(valQ, in.V, valP);
		case 2: return RGBA(valP, in.V, valT);
		case 3: return RGBA(valP, valQ, in.V);
		case 4: return RGBA(valT, valP, in.V);
		case 5: return RGBA(in.V, valP, valQ);
		default: return RGBA(0, 0, 0);
	}
}

inline CONSTEXPR_MATH OKLAB to_OKLAB(RGBA in) noexcept {
	const auto R{ OKLAB::gamma_def(in.R / 255.0f) };
	const auto G{ OKLAB::gamma_def(in.G / 255.0f) };
	const auto B{ OKLAB::gamma_def(in.B / 255.0f) };

	const auto L{ std::cbrt(0.4122214708f * R + 0.5363325363f * G + 0.0514459929f * B) };
	const auto M{ std::cbrt(0.2119034982f * R + 0.6806995451f * G + 0.1073969566f * B) };
	const auto S{ std::cbrt(0.0883024619f * R + 0.2817188376f * G + 0.6299787005f * B) };

	return OKLAB(
		0.2104542553f * L + 0.7936177850f * M - 0.0040720468f * S,
		1.9779984951f * L - 2.4285922050f * M + 0.4505937099f * S,
		0.0259040371f * L + 0.7827717662f * M - 0.8086757660f * S
	);
}

inline CONSTEXPR_MATH OKLAB to_OKLAB(OKLCH in) noexcept {
	return OKLAB(in.L, in.C * std::cos(in.H), in.C * std::sin(in.H));
}

inline CONSTEXPR_MATH OKLCH to_OKLCH(OKLAB in) noexcept {
	return OKLCH(in.L, std::sqrt(in.A * in.A + in.B * in.B), std::atan2(in.B, in.A));
}

inline CONSTEXPR_MATH OKLCH to_OKLCH(RGBA in) noexcept {
	return ::to_OKLCH(::to_OKLAB(in));
}

inline CONSTEXPR_MATH RGBA to_RGBA(OKLAB in) noexcept {
	const auto L{ std::pow(in.L + in.A * 0.39633778f + in.B * 0.21580376f, 3.0f) };
	const auto M{ std::pow(in.L - in.A * 0.10556113f - in.B * 0.06385417f, 3.0f) };
	const auto S{ std::pow(in.L - in.A * 0.08948418f - in.B * 1.29148554f, 3.0f) };

	const auto R{ +4.07674f * L - 3.30771f * M + 0.23097f * S };
	const auto G{ -1.26844f * L + 2.60976f * M - 0.34132f * S };
	const auto B{ -0.00439f * L - 0.70342f * M + 1.70758f * S };
	
	return RGBA(
		RGBA::type_C(EzMaths::round(255.0f * OKLAB::gamma_inv(R))),
		RGBA::type_C(EzMaths::round(255.0f * OKLAB::gamma_inv(G))),
		RGBA::type_C(EzMaths::round(255.0f * OKLAB::gamma_inv(B)))
	);
}

inline CONSTEXPR_MATH RGBA to_RGBA(OKLCH in) noexcept {
	return ::to_RGBA(::to_OKLAB(in));
}
