/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class DisplayColors final {
    static constexpr std::array<uint32_t, 16> BitColors{ // 0-1 classic8, 0-15 modernXO
        0xFF0C1218, 0xFFE4DCD4, 0xFF403C38, 0xFF8C8884,
        0xFFD82010, 0xFF40D020, 0xFF1040D0, 0xFFE0C818,
        0xFF501010, 0xFF105010, 0xFF50B0C0, 0xFFF08010,
        0xFFE06090, 0xFFE0F090, 0xFFB050F0, 0xFF704020,
    };
    static constexpr std::array<uint32_t, 8> ForeColors{ // 8X foreground
        0xFF000000, 0xFFFF0000, 0xFF0000FF, 0xFFFF00FF,
        0xFF00FF00, 0xFFFFFF00, 0xFF00FFFF, 0xFFFFFFFF,
    };
    static constexpr std::array<uint32_t, 4> BackColors{ // 8X background
        0xFF000060, 0xFF000000, 0xFF002000, 0xFF200000,
    };

public:
    std::array<uint32_t, 16> bit{}; // pixel bit color (planes)
    std::array<uint32_t, 10> hex{}; // hex sprite gradient map

private:
    uint32_t bgindex{}; // background color cycle index
    uint32_t megahex{}; // hex sprite color for megachip
    uint32_t buzzer{};  // buzzer color (visual beep)
    [[maybe_unused]] uint32_t _;

public:
    explicit DisplayColors();
    void setMegaHex(uint32_t);
    void setBit332(std::size_t, std::size_t);
    void cycleBackground();
    uint32_t getFore8X(const std::size_t) const;
    uint32_t getBuzzer() const;
};
