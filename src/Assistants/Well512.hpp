/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <chrono>
#include <cstdint>

class Well512 {
	using result_type = std::uint32_t;
	result_type mState[16];
	result_type mIndex{};

public:
	static constexpr result_type min() { return 0x00000000; }
	static constexpr result_type max() { return 0xFFFFFFFF; }

	Well512() {
		using chrono = std::chrono::high_resolution_clock;
		const auto seed{ chrono::now().time_since_epoch().count() };
		for (auto i{ 0 }; i < 16; ++i) {
			mState[i] = static_cast<result_type>(seed >> i);
		}
	}

	result_type get() {
		result_type a, b, c, d;

		a = mState[mIndex];
		c = mState[mIndex + 13 & 15];
		b = a ^ c ^ (a << 16) ^ (c << 15);
		c = mState[mIndex + 9 & 15];
		c = c ^ (c >> 11);
		a = mState[mIndex] = b ^ c;
		d = a ^ (a << 5 & 0xDA442D24);
		mIndex = mIndex + 15 & 15;
		a = mState[mIndex];
		mState[mIndex] = a ^ b ^ d ^ a << 2 ^ b << 18 ^ c << 28;
		return mState[mIndex];
	}

	result_type operator()() { return get(); }
};
