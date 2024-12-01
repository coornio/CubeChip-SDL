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
	#pragma region BasicKeyboard Singleton Class

class BasicKeyboard final {
	Uint8 oldState[SDL_SCANCODE_COUNT]{};

	BasicKeyboard() noexcept = default;
	~BasicKeyboard() noexcept = default;
	BasicKeyboard(const BasicKeyboard&) = delete;
	BasicKeyboard& operator=(const BasicKeyboard&) = delete;

public:
	static auto* create() noexcept {
		static BasicKeyboard self;
		return &self;
	}

	void storeOldState() noexcept;

	bool isHeldPrev(const SDL_Scancode key) const noexcept;
	bool isHeld(const SDL_Scancode key) const noexcept;
	bool isPressed(const SDL_Scancode key) const noexcept;
	bool isReleased(const SDL_Scancode key) const noexcept;

	template <std::same_as<SDL_Scancode>... S>
		requires (sizeof...(S) >= 1)
	bool areAllHeld(const S... code) const noexcept {
		return (isHeld(code) && ...);
	}

	template <std::same_as<SDL_Scancode>... S>
		requires (sizeof...(S) >= 1)
	bool areAnyHeld(const S... code) const noexcept {
		return (isHeld(code) || ...);
	}
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region BasicMouse Singleton Class

class BasicMouse final {
	Uint32 curState{}, oldState{};
	float posX{}, posY{};
	float relX{}, relY{};

	BasicMouse() = default;
	BasicMouse(const BasicMouse&) = delete;
	BasicMouse& operator=(const BasicMouse&) = delete;

public:
	static auto* create() {
		static BasicMouse self;
		return &self;
	}

	void storeOldState() noexcept;

	float getRelX() const noexcept { return relX; }
	float getRelY() const noexcept { return relY; }
	float getPosX() const noexcept { return posX; }
	float getPosY() const noexcept { return posY; }

	bool isHeldPrev(const BIC_Button key) const noexcept;
	bool isHeld(const BIC_Button key) const noexcept;
	bool isPressed(const BIC_Button key) const noexcept;
	bool isReleased(const BIC_Button key) const noexcept;

	template <std::convertible_to<BIC_Button>... S>
		requires (sizeof...(S) >= 1)
	bool areAllHeld(const S... code) const noexcept {
		return (isHeld(code) && ...);
	}

	template <std::convertible_to<BIC_Button>... S>
		requires (sizeof...(S) >= 1)
	bool areAnyHeld(const S... code) const noexcept {
		return (isHeld(code) || ...);
	}
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

namespace binput {
	extern BasicKeyboard& kb;
	extern BasicMouse&    mb;
}
