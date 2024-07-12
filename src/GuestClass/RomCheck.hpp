/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstddef>
#include <string>

inline constexpr std::size_t cexprHash(const char* str) noexcept {
	return (*str == '\0') ? 0 : cexprHash(str + 1) * 31 + *str;
}

enum RomExt : std::size_t {
	c2x = cexprHash(".c2x"), // CHIP-8X 2-page
	c4x = cexprHash(".c4x"), // CHIP-8X 4-page
	c8x = cexprHash(".c8x"), // CHIP-8X

	c8e = cexprHash(".c8e"), // CHIP-8E

	c2h = cexprHash(".c2h"), // CHIP-8 (HIRES) 2-page
	c4h = cexprHash(".c4h"), // CHIP-8 (HIRES) 4-page
	c8h = cexprHash(".c8h"), // CHIP-8 (HIRES) 2-page patched

	ch8 = cexprHash(".ch8"), // CHIP-8
	sc8 = cexprHash(".sc8"), // SUPERCHIP
	mc8 = cexprHash(".mc8"), // MEGACHIP
	gc8 = cexprHash(".gc8"), // GIGACHIP

	xo8 = cexprHash(".xo8"), // XO-CHIP
	hw8 = cexprHash(".hw8"), // HYPERWAVE-CHIP

	benchmark = cexprHash(".benchmark")
};

class RomFile final {
	 RomFile() = delete;
	~RomFile() = delete;

public:
	static std::string error;

	static bool validate(
		std::uint64_t    hash,
		std::uint64_t    size,
		std::string_view sha1 = ""
	);
};
