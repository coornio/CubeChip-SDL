/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Millis.hpp"

#include <chrono>

/*==================================================================*/

static const auto sInitialTimestamp \
	{ std::chrono::steady_clock::now() };

/*==================================================================*/

auto Millis::get() const noexcept -> return_type {
	return std::chrono::duration_cast<std::chrono::milliseconds> \
		(std::chrono::steady_clock::now() - sInitialTimestamp).count();
}

auto Millis::getSince(return_type past_millis) const noexcept -> return_type {
	return get() - past_millis;
}
