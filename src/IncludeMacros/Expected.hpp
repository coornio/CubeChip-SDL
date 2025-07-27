/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

/*==================================================================*/

#if defined(__has_include) && __has_include(<expected>) \
&& defined(__cpp_lib_expected) && (__cpp_lib_expected >= 202202L)
	#include <expected>

	template <typename T, typename E>
	using Expected = std::expected<T, E>;

	template <typename E>
	using Unexpected = std::unexpected<E>;
#else
	#include "../Libraries/tartanllama/expected.hpp"

	template <typename T, typename E>
	using Expected = tl::expected<T, E>;

	template <typename E>
	using Unexpected = tl::unexpected<E>;
#endif

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
