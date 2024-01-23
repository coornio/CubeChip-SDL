/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "BasicKeyInput.hpp"

class HexInput final {
    struct KeyInfo {
        unsigned int idx; // key index on chip8 pad
        SDL_Scancode key; // main keyboard equivalent
        SDL_Scancode alt; // alternative option
    };
    std::vector<KeyInfo> hexPad;

    SDL_Scancode _ = SDL_SCANCODE_UNKNOWN;

    const std::vector<KeyInfo> defaultKeys{
        {0x1, SDL_SCANCODE_1, _}, {0x2, SDL_SCANCODE_2, _}, {0x3, SDL_SCANCODE_3, _}, {0xC, SDL_SCANCODE_4, _},
        {0x4, SDL_SCANCODE_Q, _}, {0x5, SDL_SCANCODE_W, _}, {0x6, SDL_SCANCODE_E, _}, {0xD, SDL_SCANCODE_R, _},
        {0x7, SDL_SCANCODE_A, _}, {0x8, SDL_SCANCODE_S, _}, {0x9, SDL_SCANCODE_D, _}, {0xE, SDL_SCANCODE_F, _},
        {0xA, SDL_SCANCODE_Z, _}, {0x0, SDL_SCANCODE_X, _}, {0xB, SDL_SCANCODE_C, _}, {0xF, SDL_SCANCODE_V, _},
    };
    
    unsigned int keysCurr{}; // bitfield of key states in current frame
    unsigned int keysPrev{}; // bitfield of key states in previous frame
    unsigned int keysLock{}; // bitfield of keys excluded from input checks

public:
    explicit HexInput() : hexPad(32) {}

    unsigned int currKeys(const unsigned int idx) const {
        return (keysLock >> idx) & 0x1u;
    }

    void reset() {
        setup(defaultKeys);
    }

    void setup(const std::vector<KeyInfo>& bindings) {
        (hexPad = bindings).resize(bindings.size());
        keysPrev = keysCurr = keysLock = 0u;
    }
    
    void refresh() {
        if (!hexPad.size()) return;

        keysPrev = keysCurr;
        keysCurr = 0u;

        for (const KeyInfo& mapping : hexPad)
            if (bki::kb.isKeyHeld(mapping.key, mapping.alt))
                keysCurr |= 1u << mapping.idx;
        keysLock &= ~(keysPrev ^ keysCurr);
    }

    bool keyPressed(unsigned char& vregister) {
        if (!hexPad.size()) return false;

        const unsigned int mask{ (keysCurr & ~keysPrev) & ~keysLock };
        if (mask) {
            vregister = static_cast<unsigned char>
                (std::countr_zero(mask & ~(mask - 1u)));
            keysLock |= mask;
            return true;
        }
        return false;
    }

    bool keyPressed(const unsigned int index, const unsigned int offset) const {
        return (keysCurr & ~keysLock) & (1u << ((index & 0xFu) + offset));
    }
};
