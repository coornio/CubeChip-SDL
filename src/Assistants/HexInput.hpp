/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "BasicKeyInput.hpp"
#include <cstdint>

#define KEY(i) (SDL_SCANCODE_##i)

class HexInput final {
    struct KeyInfo {
        std::uint32_t idx; // key index on chip8 pad
        SDL_Scancode  key; // main keyboard equivalent
        SDL_Scancode  alt; // alternative option
    };
    std::vector<KeyInfo> hexPad;

    static constexpr SDL_Scancode _{ SDL_SCANCODE_UNKNOWN };

    const std::vector<KeyInfo> defaultKeys{
        {0x1, KEY(1), _}, {0x2, KEY(2), _}, {0x3, KEY(3), _}, {0xC, KEY(4), _},
        {0x4, KEY(Q), _}, {0x5, KEY(W), _}, {0x6, KEY(E), _}, {0xD, KEY(R), _},
        {0x7, KEY(A), _}, {0x8, KEY(S), _}, {0x9, KEY(D), _}, {0xE, KEY(F), _},
        {0xA, KEY(Z), _}, {0x0, KEY(X), _}, {0xB, KEY(C), _}, {0xF, KEY(V), _},
    };

    std::uint32_t keysCurr{}; // bitfield of key states in current frame
    std::uint32_t keysPrev{}; // bitfield of key states in previous frame
    std::uint32_t keysLock{}; // bitfield of keys excluded from input checks

public:
    explicit HexInput();

    void reset();
    void refresh();

    void setup(const std::vector<KeyInfo>& bindings);
    bool keyPressed(std::uint8_t& vregister);
    bool keyPressed(std::size_t index, std::size_t offset) const;
    std::uint32_t currKeys(std::size_t index) const;
};
