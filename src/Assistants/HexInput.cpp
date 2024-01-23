/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "HexInput.hpp"

HexInput::HexInput()
    : hexPad(32)
{}

void HexInput::reset() {
    setup(defaultKeys);
}

void HexInput::refresh() {
    if (!hexPad.size()) return;

    keysPrev = keysCurr;
    keysCurr = 0;

    for (const KeyInfo& mapping : hexPad)
        if (bic::kb.isKeyHeld(mapping.key, mapping.alt))
            keysCurr |= 1 << mapping.idx;
    keysLock &= ~(keysPrev ^ keysCurr);
}

void HexInput::setup(const std::vector<KeyInfo>& bindings) {
    (hexPad = bindings).resize(bindings.size());
    keysPrev = keysCurr = keysLock = 0;
}

bool HexInput::keyPressed(Uint8& vregister) {
    if (!hexPad.size()) return false;

    const auto mask{ (keysCurr & ~keysPrev) & ~keysLock };
    if (mask) {
        vregister = static_cast<Uint8>
            (std::countr_zero(mask & ~(mask - 1)));
        keysLock |= mask;
        return true;
    }
    return false;
}

bool HexInput::keyPressed(const Uint32 index, const Uint32 offset) const {
    return (keysCurr & ~keysLock) & (1 << ((index & 0xF) + offset));
}

Uint32 HexInput::currKeys(const Uint32 idx) const {
    return (keysLock >> idx) & 0x1;
}
