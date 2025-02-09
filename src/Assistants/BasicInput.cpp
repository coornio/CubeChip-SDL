/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicInput.hpp"
#include <algorithm>
#include <execution>
#include <utility>

/*==================================================================*/
	#pragma region BasicKeyboard Singleton Class

void BasicKeyboard::storeOldState() noexcept {
	std::copy_n(
		std::execution::unseq,
		SDL_GetKeyboardState(nullptr),
		0 + SDL_SCANCODE_COUNT, oldState
	);
}

bool BasicKeyboard::isHeldPrev(SDL_Scancode key) const noexcept {
	return oldState[key];
}
bool BasicKeyboard::isHeld(SDL_Scancode key) const noexcept {
	return SDL_GetKeyboardState(nullptr)[key];
}
bool BasicKeyboard::isPressed(SDL_Scancode key) const noexcept {
	return !isHeldPrev(key) && isHeld(key);
}
bool BasicKeyboard::isReleased(SDL_Scancode key) const noexcept {
	return isHeldPrev(key) && !isHeld(key);
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region BasicMouse Singleton Class

void BasicMouse::storeOldState() noexcept {
	const auto oldX{ posX };
	const auto oldY{ posY };
	oldState = SDL_GetMouseState(&posX, &posY);
	relX = posX - oldX;
	relY = posY - oldY;
}

bool BasicMouse::isHeldPrev(BIC_Button key) const noexcept {
	return oldState & key;
}
bool BasicMouse::isHeld(BIC_Button key) const noexcept {
	return SDL_GetMouseState(nullptr, nullptr) & key;
}
bool BasicMouse::isPressed(BIC_Button key) const noexcept {
	return !isHeldPrev(key) && isHeld(key);
}
bool BasicMouse::isReleased(BIC_Button key) const noexcept {
	return isHeldPrev(key) && !isHeld(key);
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
