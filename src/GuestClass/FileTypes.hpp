/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cstddef>
#include <string>

inline constexpr std::size_t cexprHash(const char* str, std::size_t v = 0) noexcept {
    return (*str == 0) ? v : 31 * cexprHash(str + 1) + *str;
}

enum FileTypes : std::size_t {
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
};

class RomFileTypes final {
    RomFileTypes() = delete;
    ~RomFileTypes() = delete;

    static std::size_t checkSize(
        const std::size_t size,
        const std::size_t offset,
        const std::size_t limit
    ) {
        return size + offset <= limit;
    }

public:
    static bool validate(
        const std::size_t hash,
        const std::size_t size,
        const std::string_view sha1 = ""
    );
};
