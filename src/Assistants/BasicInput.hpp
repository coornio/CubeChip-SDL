/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_mouse.h>

#include <concepts>
#include <algorithm>
#include <execution>

/*==================================================================*/

#define KEY(i) SDL_SCANCODE_##i
#define BTN(i) BIC_MOUSE_##i

enum BIC_Button {
	BIC_MOUSE_LEFT   = SDL_BUTTON_LMASK,
	BIC_MOUSE_RIGHT  = SDL_BUTTON_RMASK,
	BIC_MOUSE_MIDDLE = SDL_BUTTON_MMASK,
	BIC_MOUSE_X1     = SDL_BUTTON_X1MASK,
	BIC_MOUSE_X2     = SDL_BUTTON_X2MASK,
};

/*==================================================================*/
	#pragma region BasicKeyboard Class

class BasicKeyboard final {
	static constexpr auto TOTALKEYS{ 0u + SDL_SCANCODE_COUNT };

	Uint8 mOldState[TOTALKEYS]{};
	Uint8 mCurState[TOTALKEYS]{};

public:
	void updateStates() noexcept {
		std::copy_n(std::execution::unseq,
			mCurState, TOTALKEYS, mOldState);

		std::copy_n(std::execution::unseq,
			SDL_GetKeyboardState(nullptr), TOTALKEYS, mCurState);
	}

	bool isHeldPrev(SDL_Scancode key) const noexcept { return mOldState[key]; }
	bool isHeld    (SDL_Scancode key) const noexcept { return mCurState[key]; }
	bool isPressed (SDL_Scancode key) const noexcept { return !isHeldPrev(key) &&  isHeld(key); }
	bool isReleased(SDL_Scancode key) const noexcept { return  isHeldPrev(key) && !isHeld(key); }

	template <std::same_as<SDL_Scancode>... S>
		requires (sizeof...(S) >= 1)
	bool areAllHeld(S... code) const noexcept {
		return (isHeld(code) && ...);
	}

	template <std::same_as<SDL_Scancode>... S>
		requires (sizeof...(S) >= 1)
	bool areAnyHeld(S... code) const noexcept {
		return (isHeld(code) || ...);
	}
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region BasicMouse Class

class BasicMouse final {
	Uint32 mCurState{}, mOldState{};
	float mPosX{}, mPosY{};
	float mRelX{}, mRelY{};

public:
	void updateStates() noexcept {
		mOldState = mCurState;

		const auto oldX{ mPosX }, oldY{ mPosY };
		mCurState = SDL_GetMouseState(&mPosX, &mPosY);
		mRelX = mPosX - oldX; mRelY = mPosY - oldY;
	}

	float getRelX() const noexcept { return mRelX; }
	float getRelY() const noexcept { return mRelY; }
	float getPosX() const noexcept { return mPosX; }
	float getPosY() const noexcept { return mPosY; }

	bool isHeldPrev(BIC_Button key) const noexcept { return mOldState & key; }
	bool isHeld    (BIC_Button key) const noexcept { return mCurState & key; }
	bool isPressed (BIC_Button key) const noexcept { return !isHeldPrev(key) &&  isHeld(key); }
	bool isReleased(BIC_Button key) const noexcept { return  isHeldPrev(key) && !isHeld(key); }

	template <std::convertible_to<BIC_Button>... S>
		requires (sizeof...(S) >= 1)
	bool areAllHeld(S... code) const noexcept {
		return (isHeld(code) && ...);
	}

	template <std::convertible_to<BIC_Button>... S>
		requires (sizeof...(S) >= 1)
	bool areAnyHeld(S... code) const noexcept {
		return (isHeld(code) || ...);
	}
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
