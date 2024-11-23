/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <type_traits>
#include <concepts>

template<class T>
concept integral = std::is_integral_v<T>;

template<class T>
concept arithmetic = std::is_arithmetic_v<T>;

template<class T>
concept ar_pointer = std::is_pointer_v<T> && std::is_arithmetic_v<std::remove_pointer_t<T>>;

template <typename T>
concept ContiguousContainer = requires(const T & c) {
	{ std::data(c) } -> std::same_as<const typename T::value_type*>;
	{ std::size(c) } -> std::convertible_to<std::size_t>;
};
