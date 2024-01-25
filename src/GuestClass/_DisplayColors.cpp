/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Guest.hpp"

VM_Guest::DisplayColors::DisplayColors(VM_Guest& parent)
    : vm(parent)
    , bit(BitColors)
    , buzzer(bit[1])
{
    setMegaHex(0xFFFFFFFF);
}

void VM_Guest::DisplayColors::setMegaHex(const u32 color) {
    megahex = color;
    for (auto idx{ 0 }; idx < hex.size(); ++idx) {
        const float mult{ 1.0f - 0.045f * idx };
        const float R{ (color >> 16 & 0xFF) * mult * 1.03f };
        const float G{ (color >>  8 & 0xFF) * mult * 1.14f };
        const float B{ (color       & 0xFF) * mult * 1.21f };

        hex[idx] = 0xFF000000
            | as<u32>(std::min(std::roundf(R), 255.0f)) << 16
            | as<u32>(std::min(std::roundf(G), 255.0f)) <<  8
            | as<u32>(std::min(std::roundf(B), 255.0f));
    }
}

void VM_Guest::DisplayColors::setBit332(const u32 idx, const u8 color) {
    static constexpr std::array<u8, 8> map3b{ 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xFF };
    static constexpr std::array<u8, 4> map2b{ 0x00,             0x60,       0xA0,       0xFF };
    bit[idx & 0xF] = 0xFF000000
        | map3b[color >> 5 & 7] << 16 // red
        | map3b[color >> 2 & 7] << 8 // green
        | map2b[color & 3];           // blue
}

void VM_Guest::DisplayColors::cycleBackground() {
    bit[0] = BackColors[bgindex++];
    bgindex &= 0x3;
}

u32 VM_Guest::DisplayColors::getFore8X(const u8 idx) const {
    return ForeColors[idx & 0x7];
}

u32 VM_Guest::DisplayColors::getBuzzer() const {
    return buzzer;
}
