/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <fstream>
#include <limits>

#include "PathGetters.hpp"
#include "SimpleFileIO.hpp"
#include "DefaultConfig.hpp"

/*==================================================================*/

toml::table& getAppConfig() noexcept {
	static constexpr auto none{ std::numeric_limits<s32>::min() };

	static toml::table appConfig{
		{ "Window", toml::table{
			{ "Position", toml::table{
				{ "i_X", none },
				{ "i_Y", none }
			}},
			{ "Size", toml::table{
				{ "i_X",  0 },
				{ "i_Y", 0 }
			}}
		}},
		{ "Viewport", toml::table{
			{ "i_ScaleMode",  0 },
			{ "b_IntegerScaling", true },
			{ "b_UsingScanlines", true }
		}},
		{ "Audio", toml::table{
			{ "f_Volume", 0.75f },
			{ "b_Muted", false }
		}}
	};

	return appConfig;
}

auto config::writeToFile(
	const toml::table& table,
	const char* filename
) noexcept -> Expected<bool, std::error_code> {
	std::ofstream outFile(filename, std::ios::out);
	if (!outFile) { return makeUnexpected(std::make_error_code(std::errc::permission_denied)); }

	try { if (outFile << table) { return true; } else { throw std::exception(); } }
	catch (...) { return makeUnexpected(std::make_error_code(std::errc::io_error)); }
}

auto config::parseFromFile(
	const char* filename
) noexcept -> toml::parse_result {
	const auto rawData{ readFileData(filename ? filename : "") };
	const StrV tableData{ rawData ? StrV{rawData.value().data(), rawData.value().size() } : "" };

	return toml::parse(tableData);
}

/*==================================================================*/

void config::safeTableUpdate(toml::table& dst, const toml::table& src) {
	for (auto&& [key, dst_val] : dst) {
		if (const auto * src_val{ src.get(key) }) {
			if (dst_val.is_table() && src_val->is_table()) {
				safeTableUpdate(*dst_val.as_table(), *src_val->as_table());
			} else {
				if (dst_val.is_value() && src_val->is_value()
					&& dst_val.type() == src_val->type()) {
					dst.insert_or_assign(key, *src_val);
				}
			}
		}
	}
}
