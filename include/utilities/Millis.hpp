/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

/*==================================================================*/

struct Millis final {
	using return_type = long long;

	Millis() = delete;

	auto get() const noexcept -> return_type;
	auto getSince(return_type past_millis) const noexcept -> return_type;
};
