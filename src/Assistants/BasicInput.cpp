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

void BasicKeyboard::updateCopy() noexcept {
	std::copy_n(
		std::execution::par_unseq,
		SDL_GetKeyboardState(nullptr),
		0 + SDL_SCANCODE_COUNT, oldState
	);
}

bool BasicKeyboard::isHeldPrev(const SDL_Scancode key) const noexcept {
	return oldState[key];
}
bool BasicKeyboard::isHeld(const SDL_Scancode key) const noexcept {
	return SDL_GetKeyboardState(nullptr)[key];
}
bool BasicKeyboard::isPressed(const SDL_Scancode key) const noexcept {
	return !isHeldPrev(key) && isHeld(key);
}
bool BasicKeyboard::isReleased(const SDL_Scancode key) const noexcept {
	return isHeldPrev(key) && !isHeld(key);
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region BasicMouse Singleton Class

void BasicMouse::updateCopy() noexcept {
	const auto oldX{ posX };
	const auto oldY{ posY };
	oldState = SDL_GetMouseState(&posX, &posY);
	relX = posX - oldX;
	relY = posY - oldY;
}

bool BasicMouse::isHeldPrev(const BIC_Button key) const noexcept {
	return oldState & key;
}
bool BasicMouse::isHeld(const BIC_Button key) const noexcept {
	return SDL_GetMouseState(nullptr, nullptr) & key;
}
bool BasicMouse::isPressed(const BIC_Button key) const noexcept {
	return !isHeldPrev(key) && isHeld(key);
}
bool BasicMouse::isReleased(const BIC_Button key) const noexcept {
	return isHeldPrev(key) && !isHeld(key);
}


	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
