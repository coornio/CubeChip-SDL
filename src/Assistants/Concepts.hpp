/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <type_traits>
#include <iterator>
#include <concepts>

/*==================================================================*/

template <typename T>
using ValueType = typename T::value_type;

template <typename T, typename U>
concept SameValueSizes = (sizeof(ValueType<T>) == sizeof(ValueType<U>));

template <typename T, typename U>
concept SameValueTypes = std::same_as<ValueType<T>, ValueType<U>>;

template <typename T, typename U>
concept MatchingValueType = std::same_as<std::remove_cv_t<T>, std::remove_cv_t<typename U::value_type>>;

template<typename T>
concept IsPlainOldData = std::is_trivial_v<T> && std::is_standard_layout_v<T>;

template <typename T>
concept IsContiguousContainer = requires(const T& c) {
	typename T::value_type;
	requires std::same_as<
		std::remove_cv_t<typename T::value_type>,
		std::remove_cv_t<std::remove_pointer_t<decltype(std::data(c))>>
	>;
	{ std::size(c) } -> std::convertible_to<std::size_t>;
};
