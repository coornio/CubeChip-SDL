/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <chrono>

class Well512 {
	std::array<unsigned int, 16> state{};
	unsigned int index{};
public:
	using result_type = unsigned int;
	static constexpr result_type min() { return 0x00000000u; }
	static constexpr result_type max() { return 0xFFFFFFFFu; }

	Well512() {
		using chrono = std::chrono::high_resolution_clock;
		auto seed = chrono::now().time_since_epoch().count();
		for (auto& element : state)
			element = static_cast<unsigned int>(seed >>= 1);
	}

	result_type operator()() {
		unsigned int a{}, b{}, c{}, d{};

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
};
