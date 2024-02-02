/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicInput.hpp"

/*------------------------------------------------------------------*/
/*  class  BasicInput.hpp > Keyboard                                */
/*------------------------------------------------------------------*/

BasicKeyboard& BasicKeyboard::create() {
    if (!_self) _self = std::make_unique<BasicKeyboard>();
    return *_self.get();
}

std::unique_ptr<BasicKeyboard> BasicKeyboard::_self = nullptr;
BasicKeyboard& bic::kb{ BasicKeyboard::create() };

void BasicKeyboard::updateCopy() {
    std::copy_n(
        SDL_GetKeyboardState(nullptr),
        SDL_NUM_SCANCODES,
        oldState.begin()
    );
}

bool BasicKeyboard::isPrevHeld(const SDL_Scancode key) const noexcept {
    return oldState[key];
}

bool BasicKeyboard::isHeld(const SDL_Scancode key) const noexcept {
    return SDL_GetKeyboardState(nullptr)[key];
}

bool BasicKeyboard::isPressed(const SDL_Scancode key) const noexcept {
    return !isPrevHeld(key) && isHeld(key);
}

bool BasicKeyboard::isReleased(const SDL_Scancode key) const noexcept {
    return isPrevHeld(key) && !isHeld(key);
}

/*------------------------------------------------------------------*/
/*  class  BasicInput.hpp > Mouse                                   */
/*------------------------------------------------------------------*/

BasicMouse& BasicMouse::create() {
    if (!_self) _self = std::make_unique<BasicMouse>();
    return *_self.get();
}

std::unique_ptr<BasicMouse> BasicMouse::_self = nullptr;
BasicMouse& bic::mb{ BasicMouse::create() };

void BasicMouse::updateCopy() {
    const auto oldX{ posX };
    const auto oldY{ posY };
    oldState = SDL_GetMouseState(&posX, &posY);
    relX = posX - oldX;
    relY = posY - oldY;
}

Sint32 BasicMouse::getRelX() const noexcept { return relX; }
Sint32 BasicMouse::getRelY() const noexcept { return relY; }
Sint32 BasicMouse::getPosX() const noexcept { return posX; }
Sint32 BasicMouse::getPosY() const noexcept { return posY; }

bool BasicMouse::isPrevHeld(const BIC_Button key) const noexcept {
    return oldState & key;
}

bool BasicMouse::isHeld(const BIC_Button key) const noexcept {
    return SDL_GetMouseState(nullptr, nullptr) & key;
}

bool BasicMouse::isPressed(const BIC_Button key) const noexcept {
    return !isPrevHeld(key) && isHeld(key);
}

bool BasicMouse::isReleased(const BIC_Button key) const noexcept {
    return isPrevHeld(key) && !isHeld(key);
}
