/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <numeric>

#include "Typedefs.hpp"
#include "Concepts.hpp"

/*==================================================================*/

template <IsContiguousContainer Object>
	requires (!std::is_rvalue_reference_v<Object>)
inline s32 accumulate(Object& array, s32 val = {}) noexcept {
	return std::accumulate(
		std::begin(array), std::end(array), val);
}

template <IsContiguousContainer Object, typename V = ValueType<Object>>
	requires (!std::is_rvalue_reference_v<Object> && std::convertible_to<V, ValueType<Object>>)
inline void fill(Object& array, V val = {}) noexcept {
	std::fill(EXEC_POLICY(unseq)
		std::begin(array), std::end(array),
		static_cast<ValueType<Object>>(val));
}

template <IsContiguousContainer Object, typename V = ValueType<Object>>
	requires (!std::is_rvalue_reference_v<Object> && std::convertible_to<V, ValueType<Object>>)
inline void fill_n(Object& array, size_type offset = 0, size_type count = 0, V val = {}) noexcept {
	if (offset < std::size(array)) {
		std::fill_n(EXEC_POLICY(unseq)
			std::data(array) + offset, count ? count : std::size(array) - offset,
			static_cast<ValueType<Object>>(val));
	}
}

template <IsContiguousContainer Object, std::invocable Generator>
	requires (!std::is_rvalue_reference_v<Object>)
inline void generate(Object& array, Generator&& lambda) noexcept {
	std::generate(EXEC_POLICY(unseq)
		std::begin(array), std::end(array),
		std::forward<Generator>(lambda));
}

template <IsContiguousContainer Object, std::invocable Generator>
	requires (!std::is_rvalue_reference_v<Object>)
inline void generate_n(Object& array, size_type offset, size_type count, Generator&& lambda) noexcept {
	if (offset < std::size(array)) {
		std::generate_n(EXEC_POLICY(unseq)
			std::data(array) + offset,
			count ? count : std::size(array) - offset,
			std::forward<Generator>(lambda));
	}
}

/*==================================================================*/

template <typename Dst, typename Src> requires (std::convertible_to<Src, Dst>)
inline constexpr void assign_cast(Dst& dst, Src&& src) noexcept
	{ dst = static_cast<Dst>(std::forward<Src>(src)); }

template <typename Dst, typename Src> requires (std::convertible_to<Src, Dst>)
inline constexpr void assign_cast_add(Dst& dst, Src&& src) noexcept
	{ dst += static_cast<Dst>(std::forward<Src>(src)); }

template <typename Dst, typename Src> requires (std::convertible_to<Src, Dst>)
inline constexpr void assign_cast_sub(Dst& dst, Src&& src) noexcept
	{ dst -= static_cast<Dst>(std::forward<Src>(src)); }

template <typename Dst, typename Src> requires (std::convertible_to<Src, Dst>)
inline constexpr void assign_cast_mul(Dst& dst, Src&& src) noexcept
	{ dst *= static_cast<Dst>(std::forward<Src>(src)); }

template <typename Dst, typename Src> requires (std::convertible_to<Src, Dst>)
inline constexpr void assign_cast_div(Dst& dst, Src&& src) noexcept
	{ dst /= static_cast<Dst>(std::forward<Src>(src)); }

template <std::integral Dst, typename Src> requires (std::convertible_to<Src, Dst>)
inline constexpr void assign_cast_mod(Dst& dst, Src&& src) noexcept
	{ dst %= static_cast<Dst>(std::forward<Src>(src)); }

template <std::integral Dst, typename Src> requires (std::convertible_to<Src, Dst>)
inline constexpr void assign_cast_xor(Dst& dst, Src&& src) noexcept
	{ dst ^= static_cast<Dst>(std::forward<Src>(src)); }

template <std::integral Dst, typename Src> requires (std::convertible_to<Src, Dst>)
inline constexpr void assign_cast_and(Dst& dst, Src&& src) noexcept
	{ dst &= static_cast<Dst>(std::forward<Src>(src)); }

template <std::integral Dst, typename Src> requires (std::convertible_to<Src, Dst>)
inline constexpr void assign_cast_or(Dst& dst, Src&& src) noexcept
	{ dst |= static_cast<Dst>(std::forward<Src>(src)); }

template <std::integral Dst, typename Src> requires (std::convertible_to<Src, Dst>)
inline constexpr void assign_cast_shl(Dst& dst, Src&& src) noexcept
	{ dst <<= static_cast<Dst>(std::forward<Src>(src)); }

template <std::integral Dst, typename Src> requires (std::convertible_to<Src, Dst>)
inline constexpr void assign_cast_shr(Dst& dst, Src&& src) noexcept
	{ dst >>= static_cast<Dst>(std::forward<Src>(src)); }


/*==================================================================*/

inline constexpr bool simplePathValidityCheck(StrV path) noexcept {
#ifdef _WIN32
	constexpr StrV illegal{ R"(<>:"/\|?*)" };
	return !path.empty() && path.find_first_of(illegal) == StrV::npos;
#else
	return !path.empty();
#endif
}

/*==================================================================*/

template <typename E>
Unexpected<E> makeUnexpected(E&& value) {
	return Unexpected<E>(std::forward<E>(value));
}

// factory for Expected<T, E> type, expects <E> to be convertible to boolean
template <typename T, typename E>
Expected<T, E> makeExpected(T&& value, E&& error) {
	if (!error) { return std::forward<T>(value); }
	else { return makeUnexpected<E>(std::forward<E>(error)); }
}

/*==================================================================*/
