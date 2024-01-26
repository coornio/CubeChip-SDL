/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicKeyInput.hpp"

BasicKeyInput& BasicKeyInput::create() {
    if (!_self) _self = std::make_unique<BasicKeyInput>();
    return *_self.get();
}

void BasicKeyInput::updateKeyboardCopy() {
    std::copy_n(
        SDL_GetKeyboardState(nullptr),
        SDL_NUM_SCANCODES,
        oldKeyboardState.begin()
    );
}

bool BasicKeyInput::isKeyHeld(const SDL_Scancode key, const bool prev) const noexcept {
    if (prev) return oldKeyboardState[key];
    else return SDL_GetKeyboardState(nullptr)[key];
}

bool BasicKeyInput::isKeyPressed(const SDL_Scancode key) const noexcept {
    return !isKeyHeld(key, true) && isKeyHeld(key);
}

bool BasicKeyInput::isKeyReleased(const SDL_Scancode key) const noexcept {
    return isKeyHeld(key, true) && !isKeyHeld(key);
}

std::unique_ptr<BasicKeyInput> BasicKeyInput::_self = nullptr;
std::vector<Uint8> BasicKeyInput::oldKeyboardState(SDL_NUM_SCANCODES);

BasicKeyInput& bic::kb{ BasicKeyInput::create() };
