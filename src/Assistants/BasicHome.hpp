/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <filesystem>
#include <string_view>

class BasicHome {
	std::filesystem::path mHomeDirectory{};

protected:
	static bool showErrorBox(std::string_view, std::string_view);
	const auto& getHome() const noexcept { return mHomeDirectory; }

	BasicHome(const std::string_view);
};
