/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "FileTypes.hpp"

#pragma warning(push)
#pragma warning(disable : 26819) // C fallthrough warning disabled
#include "../_nlohmann/json.hpp"
#pragma warning(pop)

bool RomFileTypes::checkSize(
    const std::size_t size,
    const std::size_t offset,
    const std::size_t limit
) {
    return size + offset <= limit;
}

bool RomFileTypes::validate(
    const std::size_t hash,
    const std::size_t size,
    const std::string_view sha1
) {
    if (!sha1.empty()) {
        /* database check here */
    }

    switch (hash) {
        case (FileTypes::c2x):
        case (FileTypes::c4x):
        case (FileTypes::c8x):
            return checkSize(size, 0x300, 4'096);

        case (FileTypes::c2h):
            return checkSize(size, 0x260, 4'096);

        case (FileTypes::c4h):
            return checkSize(size, 0x244, 4'096);

        case (FileTypes::mc8):
        case (FileTypes::gc8):
            return checkSize(size, 0x200, 16'777'216);

        case (FileTypes::xo8):
        case (FileTypes::hw8):
            return checkSize(size, 0x200, 65'536);

        case (FileTypes::c8e):
        case (FileTypes::c8h): // 0x1260 at 0x200 for valid patch
        case (FileTypes::ch8):
        case (FileTypes::sc8):
        default:
            return checkSize(size, 0x200, 4'096);
    }
};
