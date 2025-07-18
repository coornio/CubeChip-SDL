/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cmath>
#include <cstdint>
#include <numbers>
#include <concepts>
#include <algorithm>

/*==================================================================*/

#ifndef __cpp_constexpr
	#define __cpp_constexpr 0
#endif

#if __cpp_constexpr >= 202211L
	#define CONSTEXPR_MATH constexpr
#else
	#define CONSTEXPR_MATH
#endif

/*==================================================================*/

struct Phase {
	using Type_B = std::uint8_t;
	using Type_F = double;

private:
	Type_F mPhase{};

public:
	template <std::integral AnyInt>
	constexpr Phase(AnyInt value) noexcept : mPhase{ Type_B(value) * (1.0 / 255.0) } {}
	constexpr Phase(Type_F value) noexcept : mPhase{ value - int(value) } {}
	constexpr Phase() noexcept = default;

	constexpr operator Type_F() const noexcept { return mPhase; }
};

class WaveForms {
	using Millis = std::uint32_t;
	using Type_B = Phase::Type_B;
	using Type_F = Phase::Type_F;

	static constexpr Type_F calc_period(Millis p, Millis t) noexcept
		{ return p ? Type_F(t % p) / p : 0.0; }

public:
	class Bipolar {
		Type_F mPhase{};

	public:
		constexpr Bipolar(Type_F value) noexcept : mPhase{ value } {}

		// cast phase value to a 0..255 value and return
		constexpr Type_B as_byte() const noexcept
			{ return Type_B(mPhase * 127.5 + 128.0); }

		// cast phase value to a 0..1 range and return
		constexpr Type_F as_unipolar() const noexcept
			{ return 0.5 * (mPhase + 1.0); }

		constexpr operator Type_F() const noexcept { return mPhase; }
	};

	/*==================================================================*/

	static CONSTEXPR_MATH Bipolar cosine(Phase phase) noexcept
		{ return std::cos(std::numbers::pi * 2.0 * phase); }
	static CONSTEXPR_MATH Bipolar cosine_t(Millis p, Millis t) noexcept
		{ return cos(calc_period(p, t)); }
	static CONSTEXPR_MATH Bipolar sine(Phase phase) noexcept
		{ return std::sin(std::numbers::pi * 2.0 * phase); }
	static CONSTEXPR_MATH Bipolar sine_t(Millis p, Millis t) noexcept
		{ return sin(calc_period(p, t)); }


	static constexpr Bipolar sawtooth(Phase phase) noexcept
		{ return 2.0 * phase - 1.0; }
	static constexpr Bipolar sawtooth_t(Millis p, Millis t) noexcept
		{ return sawtooth(calc_period(p, t)); }
	static constexpr Bipolar triangle(Phase phase) noexcept
		{ return 4.0 * (phase >= 0.5 ? 1.0 - phase : +phase) - 1.0; }
	static constexpr Bipolar triangle_t(Millis p, Millis t) noexcept
		{ return triangle(calc_period(p, t)); }


	static constexpr Bipolar pulse(Phase phase, Phase duty = 0.5) noexcept
		{ return phase >= duty ? 1.0 : -1.0; }
	static constexpr Bipolar pulse_t(Millis p, Millis t, Phase duty = 0.5) noexcept
		{ return pulse(calc_period(p, t), duty); }
	static constexpr Bipolar triangle_skew(Phase phase, Phase skew = 0.5) noexcept
		{ return skew ? 2.0 * (phase < skew ? (phase / skew) : 1.0 - (phase - skew) / (1.0 - skew)) - 1.0 : -1.0; }
	static constexpr Bipolar triangle_skew_t(Millis p, Millis t, Phase skew = 0.5) noexcept
		{ return triangle_skew(calc_period(p, t), skew); }
};

/*==================================================================*/

namespace EzMaths {
	using Type_F = Phase::Type_F;
	using Type_B = Phase::Type_B;

	class Weight {
		Type_F mWeight{};

	public:
		template <std::integral AnyInt>
		constexpr Weight(AnyInt value) noexcept : Weight(value * (1.0 / 255.0)) {}
		constexpr Weight(Type_F value) noexcept : mWeight{ std::clamp(value, 0.0, 1.0) } {}

		// cast phase value to a 0..255 value and return
		constexpr Type_B as_byte() const noexcept
			{ return Type_B((mWeight + 1.0) * 127.5); }

		constexpr operator Type_F() const noexcept { return mWeight; }
	};

	// simple constexpr-enabled fmod, internally allows s32-width division
	template <std::floating_point T>
	inline constexpr T fmod(T x, T y) noexcept
		{ return y ? x - y * int(x / y) : x; }

	template <std::floating_point T>
	inline constexpr T round(T x) noexcept {
		return x >= T(0)
			? T(static_cast<long long>(x + T(0.5)))
			: T(static_cast<long long>(x - T(0.5)));
	}

	// simple constexpr-enabled tanh approximation, mostly-par up to x of 3.0
	template <std::floating_point T>
	inline constexpr T fast_tanh(T x) noexcept
		{ return x * (T(27) + x * x) / (T(27) + T(9) * x * x); }

	inline constexpr Type_B fixedMul8(Type_B x, Type_B y) noexcept {
		return Type_B(((x * (y | y << 8)) + 0x8080u) >> 16);
	}

	inline constexpr Type_B fixedLerp8(Type_B x, Type_B y, Weight w) noexcept {
		return Type_B(fixedMul8(x, 255u - w.as_byte()) + fixedMul8(y, w.as_byte()));
	}

	template <std::integral T>
	inline constexpr T fixedLerpN(T x, T y, Weight w, T full_hue, T half_hue) noexcept {
		const auto shortest{ (y - x + half_hue) % full_hue - half_hue };
		return T((x + T(shortest * w) + full_hue) % full_hue);
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
	using Type_C = std::uint8_t;
	using Packed = std::uint32_t;
	using Weight = EzMaths::Weight;

	Type_C R{}, G{}, B{}, A{};

	constexpr RGBA() noexcept = default;
	constexpr RGBA(Packed color) noexcept
		: R{ Type_C(color >> 24) }
		, G{ Type_C(color >> 16) }
		, B{ Type_C(color >>  8) }
		, A{ Type_C(color >>  0) }
	{}
	constexpr RGBA(Type_C R, Type_C G, Type_C B, Type_C A = 0xFF) noexcept
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
	using Type_H = std::int16_t;
	using Type_S = std::uint8_t;
	using Type_V = Type_S;
	using Packed = std::uint32_t;
	using Weight = EzMaths::Weight;

	static constexpr auto full_hue{ Type_H(0x600u) };
	static constexpr auto half_hue{ Type_H(full_hue >> 1) };

	Type_H H{};
	Type_S S{};
	Type_V V{};

	constexpr HSV() noexcept = default;
	constexpr HSV(Packed color) noexcept
		: H{ Type_H(color >> 16) }
		, S{ Type_S(color >>  8) }
		, V{ Type_V(color >>  0) }
	{}
	constexpr HSV(Type_H H, Type_S S, Type_V V) noexcept
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
	using Type_F = EzMaths::Type_F;
	using Weight = EzMaths::Weight;

	Type_F L{}, A{}, B{};

	constexpr OKLAB() noexcept = default;
	constexpr OKLAB(Type_F L, Type_F A, Type_F B) noexcept
		: L{ L }, A{ A }, B{ B }
	{}

	static CONSTEXPR_MATH Type_F gamma_def(Type_F x) noexcept {
		return x <= 0.0404500 ? x / 12.92 : std::pow((x + 0.055) / 1.055, 2.4);
		//return std::pow(x, 2.2); // quick path
	}
	static CONSTEXPR_MATH Type_F gamma_inv(Type_F x) noexcept {
		return x <= 0.0031308 ? x * 12.92 : 1.055 * std::pow(x, 1.0 / 2.4) - 0.055;
		//return std::pow(x, 1.0 / 2.2); // quick path
	}

	static constexpr OKLAB lerp(OKLAB x, OKLAB y, Weight w) noexcept {
		return OKLAB(
			std::lerp(x.L, y.L, w),
			std::lerp(x.A, y.A, w),
			std::lerp(x.B, y.B, w));
	}

	static CONSTEXPR_MATH RGBA lerp(RGBA x, RGBA y, Weight w) noexcept {
		return ::to_RGBA(lerp(::to_OKLAB(x), ::to_OKLAB(y), w));
	}
};

struct OKLCH {
	using Type_F = OKLAB::Type_F;
	using Weight = OKLAB::Weight;

	Type_F L{}, C{}, H{};

	constexpr OKLCH() noexcept = default;
	constexpr OKLCH(Type_F L, Type_F C, Type_F H) noexcept
		: L{ L }, C{ C }, H{ H }
	{}

	static constexpr OKLCH lerp(OKLCH x, OKLCH y, Weight w) noexcept {
		const auto delta{ EzMaths::fmod(y.H - x.H + \
			std::numbers::pi * 3, std::numbers::pi * 2) - std::numbers::pi };
		return OKLCH(
			std::lerp(x.L, y.L, w),
			std::lerp(x.C, y.C, w),
			y.H + delta * w);
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

	return HSV(HSV::Type_H((hueV + HSV::full_hue) % HSV::full_hue), \
		HSV::Type_S((diff * 0xFF + (maxV >> 1)) / maxV), HSV::Type_V(maxV));
}

inline constexpr RGBA to_RGBA(HSV in) noexcept {
	if (in.S == 0x00) [[unlikely]]
		{ return RGBA(in.V, in.V, in.V); }

	const auto hueV{ in.H & 0xFF };

	const auto valP{ RGBA::Type_C((in.V * (0x00FF - in.S)                  + 0x007F) / 0x00FF) };
	const auto valQ{ RGBA::Type_C((in.V * (0xFF00 - in.S *          hueV)  + 0x7FFF) / 0xFF00) };
	const auto valT{ RGBA::Type_C((in.V * (0xFF00 - in.S * (0x100 - hueV)) + 0x7FFF) / 0xFF00) };

	switch (in.H >> 8) {
		case 0:  return RGBA(in.V, valT, valP);
		case 1:  return RGBA(valQ, in.V, valP);
		case 2:  return RGBA(valP, in.V, valT);
		case 3:  return RGBA(valP, valQ, in.V);
		case 4:  return RGBA(valT, valP, in.V);
		case 5:  return RGBA(in.V, valP, valQ);
		default: return RGBA(0, 0, 0);
	}
}

inline CONSTEXPR_MATH OKLAB to_OKLAB(RGBA in) noexcept {
	const auto R{ OKLAB::gamma_def(in.R / 255.0f) };
	const auto G{ OKLAB::gamma_def(in.G / 255.0f) };
	const auto B{ OKLAB::gamma_def(in.B / 255.0f) };

	const auto L{ std::cbrt(0.4122214708 * R + 0.5363325363 * G + 0.0514459929 * B) };
	const auto M{ std::cbrt(0.2119034982 * R + 0.6806995451 * G + 0.1073969566 * B) };
	const auto S{ std::cbrt(0.0883024619 * R + 0.2817188376 * G + 0.6299787005 * B) };

	return OKLAB(
		0.2104542553 * L + 0.7936177850 * M - 0.0040720468 * S,
		1.9779984951 * L - 2.4285922050 * M + 0.4505937099 * S,
		0.0259040371 * L + 0.7827717662 * M - 0.8086757660 * S
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
	const auto L{ std::pow(in.L + in.A * 0.39633778 + in.B * 0.21580376, 3.0) };
	const auto M{ std::pow(in.L - in.A * 0.10556113 - in.B * 0.06385417, 3.0) };
	const auto S{ std::pow(in.L - in.A * 0.08948418 - in.B * 1.29148554, 3.0) };

	const auto R{ +4.07674 * L - 3.30771 * M + 0.23097 * S };
	const auto G{ -1.26844 * L + 2.60976 * M - 0.34132 * S };
	const auto B{ -0.00439 * L - 0.70342 * M + 1.70758 * S };
	
	return RGBA(
		RGBA::Type_C(EzMaths::round(255.0 * OKLAB::gamma_inv(R))),
		RGBA::Type_C(EzMaths::round(255.0 * OKLAB::gamma_inv(G))),
		RGBA::Type_C(EzMaths::round(255.0 * OKLAB::gamma_inv(B)))
	);
}

inline CONSTEXPR_MATH RGBA to_RGBA(OKLCH in) noexcept {
	return ::to_RGBA(::to_OKLAB(in));
}
