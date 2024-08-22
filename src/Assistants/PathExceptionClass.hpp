/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <filesystem>
#include <string_view>
#include <stdexcept>

struct PathException : public std::runtime_error {
	PathException(
		const std::string_view       e,
		const std::filesystem::path& p
	) : runtime_error{ e.data() + p.string() } {}
};
