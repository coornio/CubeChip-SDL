/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "EzMaths.hpp"

#include <cmath>
#include <numbers>

/*==================================================================*/

struct Phase {
	using Byte_T  = EzMaths::u8;
	using Float_T = EzMaths::f64;

private:
	Float_T mPhase{};

public:
	template <std::integral Any_Int>
	constexpr Phase(Any_Int value) noexcept : mPhase{ Byte_T(value) * (1.0 / 255.0) } {}
	constexpr Phase(Float_T value) noexcept : mPhase{ value - int(value) } {}
	constexpr Phase() noexcept = default;

	constexpr operator Float_T() const noexcept { return mPhase; }
};

class WaveForms {
	using Millis  = EzMaths::u32;
	using Byte_T  = Phase::Byte_T;
	using Float_T = Phase::Float_T;

	static constexpr Float_T calc_period(Millis p, Millis t) noexcept
		{ return p ? Float_T(t % p) / p : 0.0; }

public:
	class Bipolar {
		Float_T mPhase{};

	public:
		constexpr Bipolar(Float_T value) noexcept : mPhase{ value } {}

		// cast phase value to a 0..255 value and return
		constexpr Byte_T as_byte() const noexcept
			{ return Byte_T(mPhase * 127.5 + 128.0); }

		// cast phase value to a 0..1 range and return
		constexpr Float_T as_unipolar() const noexcept
			{ return 0.5 * (mPhase + 1.0); }

		constexpr operator Float_T() const noexcept { return mPhase; }
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
