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

class BasicKeyInput final {
    static std::unique_ptr<BasicKeyInput> _self;
    static std::vector<Uint8> oldState;

    BasicKeyInput() {};
    BasicKeyInput(const BasicKeyInput&) = delete;
    BasicKeyInput& operator=(const BasicKeyInput&) = delete;
    friend std::unique_ptr<BasicKeyInput> std::make_unique<BasicKeyInput>();

public:
    static BasicKeyInput& create();
    static void updateCopy();

    bool isPrevHeld(SDL_Scancode) const noexcept;
    bool isHeld(SDL_Scancode)     const noexcept;
    bool isPressed(SDL_Scancode)  const noexcept;
    bool isReleased(SDL_Scancode) const noexcept;

    template <std::same_as<SDL_Scancode>... S>
        requires (sizeof...(S) >= 2)
    bool areAllHeld(S... code) const noexcept {
        const auto* const state{ SDL_GetKeyboardState(nullptr) };
        return (state[code] && ...);
    }

    template <std::same_as<SDL_Scancode>... S>
        requires (sizeof...(S) >= 2)
    bool areAnyHeld(S... code) const noexcept {
        const auto* const state{ SDL_GetKeyboardState(nullptr) };
        return (state[code] || ...);
    }
};

class BasicMouseInput final {
    static std::unique_ptr<BasicMouseInput> _self;
    static Uint32 oldState;

    BasicMouseInput() {};
    BasicMouseInput(const BasicMouseInput&) = delete;
    BasicMouseInput& operator=(const BasicMouseInput&) = delete;
    friend std::unique_ptr<BasicMouseInput> std::make_unique<BasicMouseInput>();

public:
    static BasicMouseInput& create();
    static void updateCopy();

    bool isPrevHeld(BIC_Button) const noexcept;
    bool isHeld(BIC_Button)     const noexcept;
    bool isPressed(BIC_Button)  const noexcept;
    bool isReleased(BIC_Button) const noexcept;

    template <std::same_as<BIC_Button>... S>
        requires (sizeof...(S) >= 2)
    bool areAllHeld(S... code) const noexcept {
        const auto state{ SDL_GetMouseState(nullptr, nullptr) };
        return ((state & code) && ...);
    }

    template <std::same_as<BIC_Button>... S>
        requires (sizeof...(S) >= 2)
    bool areAnyHeld(S... code) const noexcept {
        const auto state{ SDL_GetMouseState(nullptr, nullptr) };
        return ((state & code) || ...);
    }
};

namespace bic { // basic input class
    extern BasicKeyInput& kb;
    extern BasicMouseInput& mb;
}
