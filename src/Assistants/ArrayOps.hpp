/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <numeric>
#include <cstddef>
#include <algorithm>

#include "Concepts.hpp"
#include "../IncludeMacros/ExecPolicy.hpp"

/*==================================================================*/

template <IsContiguousContainer Object>
	requires (!std::is_rvalue_reference_v<Object>)
inline signed accumulate(Object& array, signed val = {}) noexcept {
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
inline void fill_n(Object& array, std::size_t offset = 0, std::size_t count = 0, V val = {}) noexcept {
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
inline void generate_n(Object& array, std::size_t offset, std::size_t count, Generator&& lambda) noexcept {
	if (offset < std::size(array)) {
		std::generate_n(EXEC_POLICY(unseq)
			std::data(array) + offset,
			count ? count : std::size(array) - offset,
			std::forward<Generator>(lambda));
	}
}
