/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL_keyboard.h>
#include <SDL_scancode.h>
#include <SDL_mouse.h>

#include <vector>
#include <memory>

#define KEY(i) SDL_SCANCODE_##i
#define BTN(i) BIC_MOUSE_##i

enum BIC_Button {
    BIC_MOUSE_LEFT   = SDL_BUTTON_LMASK,
    BIC_MOUSE_RIGHT  = SDL_BUTTON_RMASK,
    BIC_MOUSE_MIDDLE = SDL_BUTTON_MMASK,
    BIC_MOUSE_X1     = SDL_BUTTON_X1MASK,
    BIC_MOUSE_X2     = SDL_BUTTON_X2MASK,
};

/*------------------------------------------------------------------*/
/*  singleton class  Keyboard                                       */
/*------------------------------------------------------------------*/

class BasicKeyboard final {
    static std::unique_ptr<BasicKeyboard> _self;
    std::vector<Uint8> oldState;

    BasicKeyboard() : oldState(SDL_NUM_SCANCODES) {};
    BasicKeyboard(const BasicKeyboard&) = delete;
    BasicKeyboard& operator=(const BasicKeyboard&) = delete;
    friend std::unique_ptr<BasicKeyboard> std::make_unique<BasicKeyboard>();

public:
    static BasicKeyboard& create();

    void updateCopy();
    bool isPrevHeld(SDL_Scancode) const noexcept;
    bool isHeld(SDL_Scancode)     const noexcept;
    bool isPressed(SDL_Scancode)  const noexcept;
    bool isReleased(SDL_Scancode) const noexcept;

    template <std::same_as<SDL_Scancode>... S>
        requires (sizeof...(S) >= 1)
    bool areAllHeld(const S... code) const noexcept {
        const auto* const state{ SDL_GetKeyboardState(nullptr) };
        return (state[code] && ...) && true;
    }

    template <std::same_as<SDL_Scancode>... S>
        requires (sizeof...(S) >= 1)
    bool areAnyHeld(const S... code) const noexcept {
        const auto* const state{ SDL_GetKeyboardState(nullptr) };
        return (state[code] || ...);
    }
};

/*------------------------------------------------------------------*/
/*  singleton class  Mouse                                          */
/*------------------------------------------------------------------*/

class BasicMouse final {
    static std::unique_ptr<BasicMouse> _self;
    Uint32 curState{}, oldState{};
    Sint32 posX{}, posY{};
    Sint32 relX{}, relY{};

    BasicMouse() {};
    BasicMouse(const BasicMouse&) = delete;
    BasicMouse& operator=(const BasicMouse&) = delete;
    friend std::unique_ptr<BasicMouse> std::make_unique<BasicMouse>();

public:
    static BasicMouse& create();

    void updateCopy();
    bool isPrevHeld(BIC_Button) const noexcept;
    bool isHeld(BIC_Button)     const noexcept;
    bool isPressed(BIC_Button)  const noexcept;
    bool isReleased(BIC_Button) const noexcept;

    Sint32 getRelX() const noexcept;
    Sint32 getRelY() const noexcept;
    Sint32 getPosX() const noexcept;
    Sint32 getPosY() const noexcept;

    template <std::same_as<BIC_Button>... S>
        requires (sizeof...(S) >= 1)
    bool areAllHeld(const S... code) const noexcept {
        const auto state{ SDL_GetMouseState(nullptr, nullptr) };
        return ((state & code) && ...) && true;
    }

    template <std::same_as<BIC_Button>... S>
        requires (sizeof...(S) >= 1)
    bool areAnyHeld(const S... code) const noexcept {
        const auto state{ SDL_GetMouseState(nullptr, nullptr) };
        return ((state & code) || ...);
    }
};

namespace bic { // basic input class
    extern BasicKeyboard& kb;
    extern BasicMouse&    mb;
}
