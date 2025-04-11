/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <fstream>

#include "Misc.hpp"
#include "../Libraries/toml++/toml.hpp"

/*==================================================================*/

namespace toml {
	inline auto writeToFile(
		const Path& filePath,
		const toml::table& tomlTable
	) noexcept -> Expected<bool, std::error_code> {
		std::ofstream outFile(filePath, std::ios::out);
		if (!outFile) { return std::unexpected(std::make_error_code(std::errc::permission_denied)); }

		try {
			outFile << tomlTable;
			if (outFile.good()) { return true; }
			else { throw std::exception(); }
		}
		catch (...) { return std::unexpected(std::make_error_code(std::errc::io_error)); }
	}
}
