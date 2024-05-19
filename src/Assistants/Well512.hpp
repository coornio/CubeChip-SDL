/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <chrono>
#include <cstdint>
#include <array>

class Well512 {
	std::array<std::uint32_t, 16> state{};
	std::uint32_t index{};

public:
	using result_type = std::uint32_t;
	static constexpr result_type min() { return 0x00000000u; }
	static constexpr result_type max() { return 0xFFFFFFFFu; }

	Well512() {
		using chrono = std::chrono::high_resolution_clock;
		auto seed = chrono::now().time_since_epoch().count();
		for (auto& element : state) {
			element = static_cast<std::uint32_t>(seed >>= 1);
		}
	}

	result_type get() {
		std::uint32_t a{}, b{}, c{}, d{};

		a = state[index];
		c = state[(index + 13u) & 15u];
		b = a ^ c ^ (a << 16u) ^ (c << 15u);
		c = state[(index + 9u) & 15u];
		c = c ^ (c >> 11u);
		a = state[index] = b ^ c;
		d = a ^ ((a << 5u) & 0xDA442D24u);
		index = (index + 15u) & 15u;
		a = state[index];
		state[index] = a ^ b ^ d ^ (a << 2u) ^ (b << 18u) ^ (c << 28u);
		return state[index];
	}

	result_type operator()() {
		return get();
	}
};
