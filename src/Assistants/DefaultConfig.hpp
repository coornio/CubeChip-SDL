/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Misc.hpp"

#define TOML_EXCEPTIONS 0
#include "../Libraries/toml++/toml.hpp"

/*==================================================================*/

namespace config {
	inline const char* HomePath{};
	inline const char* FilePath{};
	inline       bool  Portable{};
}

/*==================================================================*/

toml::table& getAppConfig() noexcept;

/*==================================================================*/

namespace config {
	// copies key values from the right table to the left, if they exist already
	void safeTableUpdate(toml::table& dst, const toml::table& src);

	// expects full filename/path
	auto writeToFile(
		const toml::table& table,
		const char* filename
	) noexcept -> Expected<bool, std::error_code>;

	// expects full filename/path
	auto parseFromFile(const char* filename) noexcept -> toml::parse_result;
}

namespace config {
	template <typename T>
	inline T get(const toml::table& table, StrV path, T value = T{}) {
		const auto val{ table.at_path(path).value<T>() };
		return val ? static_cast<T>(*val) : value;
	}

	template <typename T>
	inline void set(toml::table& root, StrV path, T value = T{}) {
		auto* current{ &root };
		auto start{ path.begin() };

		while (start != path.end()) {
			auto end{ std::find(start, path.end(), '.') };
			StrV key{ start, end };

			if (end == path.end()) {
				current->insert_or_assign(key, value);
				return;
			}

			if (!current->contains(key)) {
				current->insert(key, toml::table{});
			}

			current = current->get(key)->as_table();
			if (!current) { return; }
			else { start = end + 1; }
		}
	}
}
