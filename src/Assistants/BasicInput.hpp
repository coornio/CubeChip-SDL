/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_mouse.h>

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

class alignas(32) BasicKeyboard final {
	static BasicKeyboard _self;
	Uint8 oldState[SDL_NUM_SCANCODES]{};

	BasicKeyboard() {};
	BasicKeyboard(const BasicKeyboard&) = delete;
	BasicKeyboard& operator=(const BasicKeyboard&) = delete;

public:
	static BasicKeyboard& create();

	void updateCopy();
	bool isHeldPrev(const SDL_Scancode key) const noexcept {
		return oldState[key];
	}
	bool isHeld(const SDL_Scancode key) const noexcept {
		return SDL_GetKeyboardState(nullptr)[key];
	}
	bool isPressed(const SDL_Scancode key) const noexcept {
		return !isHeldPrev(key) && isHeld(key);
	}
	bool isReleased(const SDL_Scancode key) const noexcept {
		return isHeldPrev(key) && !isHeld(key);
	}

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

class alignas(8) BasicMouse final {
	static BasicMouse _self;
	Uint32 curState{}, oldState{};
	float posX{}, posY{};
	float relX{}, relY{};

	BasicMouse() {};
	BasicMouse(const BasicMouse&) = delete;
	BasicMouse& operator=(const BasicMouse&) = delete;

public:
	static BasicMouse& create();

	void updateCopy();

	float getRelX() const noexcept { return relX; }
	float getRelY() const noexcept { return relY; }
	float getPosX() const noexcept { return posX; }
	float getPosY() const noexcept { return posY; }

	bool isHeldPrev(const BIC_Button key) const noexcept {
		return oldState & key;
	}
	bool isHeld(const BIC_Button key) const noexcept {
		return SDL_GetMouseState(nullptr, nullptr) & key;
	}
	bool isPressed(const BIC_Button key) const noexcept {
		return !isHeldPrev(key) && isHeld(key);
	}
	bool isReleased(const BIC_Button key) const noexcept {
		return isHeldPrev(key) && !isHeld(key);
	}

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
