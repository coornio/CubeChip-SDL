/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <chrono>
#include <cstddef>
#include <concepts>

class Well512 {
	using result_type = unsigned int;
	result_type mIndex{};
	result_type mState[16];

public:
	static constexpr result_type min() noexcept { return 0x00000000; }
	static constexpr result_type max() noexcept { return 0xFFFFFFFF; }

	Well512() noexcept {
		using chrono = std::chrono::steady_clock;
		const auto seed{ chrono::now().time_since_epoch().count() };
		for (auto i{ 0 }; i < 16; ++i) {
			mState[i] = static_cast<result_type>(seed >> i * 2);
		}
	}

	template <typename T, std::size_t N>
		requires (std::is_arithmetic_v<T> && N >= 16)
	constexpr Well512(T(&seeds)[N]) noexcept {
		for (auto i{ 0 }; i < 16; ++i) {
			mState[i] = static_cast<result_type>(seeds[i]);
		}
	}

	constexpr operator result_type() noexcept
		{ return get<result_type>(); }

	template <typename T>
		requires (std::is_arithmetic_v<T>)
	constexpr T get() noexcept {
		result_type a, b, c, d;

		a = mState[mIndex];
		c = mState[(mIndex + 13) & 0xF];
		b = a ^ c ^ (a << 16) ^ (c << 15);
		c = mState[(mIndex + 9) & 0xF];
		c = c ^ (c >> 11);
		a = mState[mIndex] = b ^ c;
		d = a ^ (a << 5 & 0xDA442D24);
		mIndex = (mIndex + 15) & 0xF;
		a = mState[mIndex];
		mState[mIndex] = a ^ b ^ d ^ a << 2 ^ b << 18 ^ c << 28;
		return static_cast<T>(mState[mIndex]);
	}
};
