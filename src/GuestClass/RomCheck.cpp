/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "RomCheck.hpp"

#pragma warning(push)
#pragma warning(disable : 26819) // C fallthrough warning disabled
#include "../_nlohmann/json.hpp"
#pragma warning(pop)

std::string RomFile::error{};

bool RomFile::validate(
	const std::uint64_t    hash,
	const std::uint64_t    size,
	const std::string_view sha1
) {
	const auto checkSize{ [&size](
		const std::uint64_t offset,
		const std::uint64_t limit
	) -> bool {
		const bool valid{ size + offset <= limit };
		if (!valid) { error = "file size exceeds platform limits"; }
		return valid;
	} };

	if (!sha1.empty()) {
		/* database check here */
	}

	switch (hash) {
		case (RomExt::c2x):
		case (RomExt::c4x):
		case (RomExt::c8x):
			return checkSize(0x300, 4'096);

		case (RomExt::c2h):
			return checkSize(0x260, 4'096);

		case (RomExt::c4h):
			return checkSize(0x244, 4'096);

		case (RomExt::mc8):
		case (RomExt::gc8):
			return checkSize(0x200, 16'777'216);

		case (RomExt::xo8):
		case (RomExt::hw8):
			return checkSize(0x200, 65'536);

		case (RomExt::c8e):
		case (RomExt::c8h):
		case (RomExt::ch8):
		case (RomExt::sc8):
			return checkSize(0x200, 4'096);

		case (RomExt::benchmark):
			return checkSize(0x200, 65'536);

		default:
			error = "unknown filetype or platform";
			return false;
	}
};
