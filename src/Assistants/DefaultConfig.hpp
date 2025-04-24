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
	// copies key values from the right table to the left, if they exist already
	void safeTableUpdate(toml::table& dst, const toml::table& src);
	void safeTableInsert(toml::table& dst, const toml::table& src);

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
	inline void get(const toml::table& src, StrV key, T& dst) {
		if (auto val = src.at_path(key).value<T>())
			{ dst = static_cast<T>(*val); }
	}

	template <typename T>
	inline void set(toml::table& dst, StrV key, T src = T{}) {
		auto* current{ &dst };
		auto start{ key.begin() };

		while (start != key.end()) {
			auto end{ std::find(start, key.end(), '.') };
			StrV subkey{ start, end };

			if (end == key.end()) {
				current->insert_or_assign(subkey, src);
				return;
			}

			if (!current->contains(subkey)) {
				current->insert(subkey, toml::table{});
			}

			current = current->get(subkey)->as_table();
			if (!current) { return; }
			else { start = end + 1; }
		}
	}
}
