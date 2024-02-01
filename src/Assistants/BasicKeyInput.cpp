/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicKeyInput.hpp"

/*------------------------------------------------------------------*/
/*  class  BasicKeyInput                                            */
/*------------------------------------------------------------------*/

BasicKeyInput& BasicKeyInput::create() {
    if (!_self) _self = std::make_unique<BasicKeyInput>();
    return *_self.get();
}

std::unique_ptr<BasicKeyInput> BasicKeyInput::_self = nullptr;
std::vector<Uint8> BasicKeyInput::oldState(SDL_NUM_SCANCODES);
BasicKeyInput& bic::kb{ BasicKeyInput::create() };

void BasicKeyInput::updateCopy() {
    std::copy_n(
        SDL_GetKeyboardState(nullptr),
        SDL_NUM_SCANCODES,
        oldState.begin()
    );
}

bool BasicKeyInput::isPrevHeld(const SDL_Scancode key) const noexcept {
    return oldState[key];
}

bool BasicKeyInput::isHeld(const SDL_Scancode key) const noexcept {
    return SDL_GetKeyboardState(nullptr)[key];
}

bool BasicKeyInput::isPressed(const SDL_Scancode key) const noexcept {
    return !isPrevHeld(key) && isHeld(key);
}

bool BasicKeyInput::isReleased(const SDL_Scancode key) const noexcept {
    return isPrevHeld(key) && !isHeld(key);
}

/*------------------------------------------------------------------*/
/*  class  BasicMouseInput                                          */
/*------------------------------------------------------------------*/

BasicMouseInput& BasicMouseInput::create() {
    if (!_self) _self = std::make_unique<BasicMouseInput>();
    return *_self.get();
}

std::unique_ptr<BasicMouseInput> BasicMouseInput::_self = nullptr;
Uint32 BasicMouseInput::oldState{};
BasicMouseInput& bic::mb{ BasicMouseInput::create() };

void BasicMouseInput::updateCopy() {
    oldState = SDL_GetMouseState(nullptr, nullptr);
}

bool BasicMouseInput::isPrevHeld(const BIC_Button key) const noexcept {
    return oldState & key;
}

bool BasicMouseInput::isHeld(const BIC_Button key) const noexcept {
    return SDL_GetMouseState(nullptr, nullptr) & key;
}

bool BasicMouseInput::isPressed(const BIC_Button key) const noexcept {
    return !isPrevHeld(key) && isHeld(key);
}

bool BasicMouseInput::isReleased(const BIC_Button key) const noexcept {
    return isPrevHeld(key) && !isHeld(key);
}
