/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Typedefs.hpp"
#include "Concepts.hpp"

/*==================================================================*/

inline constexpr u32 KiB{ 1024 };
inline constexpr u32 MiB{ 1024 * KiB };
inline constexpr u32 GiB{ 1024 * MiB };

inline constexpr u32 CalcBytes(u32 value, u32 unit) noexcept {
	return value * unit;
}

/*==================================================================*/

template <IsContiguousContainer Object>
	requires (!std::is_rvalue_reference_v<Object>)
inline void initialize(Object& array) noexcept {
	std::fill(EXEC_POLICY(unseq)
		std::begin(array), std::end(array), typename Object::value_type{});
}

/*==================================================================*/

inline constexpr auto intByteMult(u32 color1, u32 color2) noexcept {
	return static_cast<u8>(((color1 * (color2 | color2 << 8)) + 0x8080) >> 16);
}

/*==================================================================*/

#if defined(__has_include) && __has_include(<expected>) \
	&& defined(__cpp_lib_expected) && (__cpp_lib_expected >= 202202L)
	#include <expected>
	namespace expected_ns = std;

	template <typename T, typename E>
	using Expected = std::expected<T, E>;

	// factory for Expected<T, E> type, <E> should be able to override as a boolean.
	template <typename T, typename E>
	Expected<T, E> makeExpected(T&& value, E&& error) {
		if (error) { return std::unexpected(error); }
		else { return std::forward<T&&>(value); }
	}

	template <typename T>
	using Unexpected = std::unexpected<T>;
#else
	#include "../Libraries/tartanllama/expected.hpp"
	namespace expected_ns = tl;

	template <typename T, typename E>
	using Expected = tl::expected<T, E>;

	// factory for Expected<T, E> type, <E> should be able to override as a boolean.
	template <typename T, typename E>
	Expected<T, E> makeExpected(T&& value, E&& error) {
		if (error) { return tl::unexpected(error); }
		else { return std::forward<T&&>(value); }
	}

	template <typename T>
	using Unexpected = tl::unexpected<T>;
#endif
