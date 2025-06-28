/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstdint>
#include <cstddef>

template<typename T> requires std::is_integral_v<T>
class BytePun {
	T mInt{};

public:
	BytePun(const T value) : mInt{ value } {}
	operator T() const { return mInt; }

	auto* operator[](std::size_t const idx) {
		return reinterpret_cast<std::uint8_t*>(&mInt + idx);
	}
	auto const* operator[](std::size_t const idx) const {
		return reinterpret_cast<std::uint8_t const*>(&mInt + idx);
	}

	BytePun& operator+=(const IsArithmetic auto rhs) { mInt += rhs; return *this; }
	BytePun& operator-=(const IsArithmetic auto rhs) { mInt -= rhs; return *this; }
	BytePun& operator*=(const IsArithmetic auto rhs) { mInt *= rhs; return *this; }
	BytePun& operator/=(const IsArithmetic auto rhs) { mInt /= rhs; return *this; }
	BytePun& operator%=(const IsArithmetic auto rhs) { mInt %= rhs; return *this; }

	BytePun& operator&= (const IsIntegral auto rhs) { mInt  &= rhs; return *this; }
	BytePun& operator|= (const IsIntegral auto rhs) { mInt  |= rhs; return *this; }
	BytePun& operator^= (const IsIntegral auto rhs) { mInt  ^= rhs; return *this; }
	BytePun& operator<<=(const IsIntegral auto rhs) { mInt <<= rhs; return *this; }
	BytePun& operator>>=(const IsIntegral auto rhs) { mInt >>= rhs; return *this; }
};
