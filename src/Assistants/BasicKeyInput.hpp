/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL_keyboard.h>
#include <SDL_scancode.h>

#include <vector>
#include <ranges>
#include <memory>
#include <algorithm>

template <typename R, typename T>
concept RangeOf = std::ranges::range<R> &&
std::convertible_to<std::ranges::range_value_t<R>, T>;

class BasicKeyInput {
    static std::vector<Uint8> oldKeyboardState;
    static std::unique_ptr<BasicKeyInput> _self;

    BasicKeyInput() {};
    BasicKeyInput(const BasicKeyInput&) = delete;
    BasicKeyInput& operator=(const BasicKeyInput&) = delete;
    friend std::unique_ptr<BasicKeyInput> std::make_unique<BasicKeyInput>();

public:
    static BasicKeyInput& create();
    static void updateKeyboardCopy();

    bool isKeyHeld(SDL_Scancode, const bool = false) const noexcept;
    bool isKeyPressed(SDL_Scancode) const noexcept;
    bool isKeyReleased(SDL_Scancode) const noexcept;

    template <RangeOf<SDL_Scancode> R>
    bool areKeysHeld(const R& keys) const noexcept {
        return std::ranges::any_of(keys, [](const SDL_Scancode code) {
            return code && SDL_GetKeyboardState(nullptr)[code];
        });
    }

    template <std::same_as<SDL_Scancode>... S>
    bool areKeysHeld(S... keys) const noexcept {
        return areKeysHeld(std::array{ keys... });
    }
};

namespace bki {
    extern BasicKeyInput& kb;
}
