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
/*==================================================================*/

BasicKeyboard& bic::kb{ *BasicKeyboard::create() };

void BasicKeyboard::updateCopy() {
	std::copy_n(
		std::execution::par_unseq,
		SDL_GetKeyboardState(nullptr),
		static_cast<Uint32>(SDL_NUM_SCANCODES),
		oldState.data()
	);
}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region BasicMouse Singleton Class
/*==================================================================*/

BasicMouse& bic::mb{ *BasicMouse::create() };

void BasicMouse::updateCopy() {
	const auto oldX{ posX };
	const auto oldY{ posY };
	oldState = SDL_GetMouseState(&posX, &posY);
	relX = posX - oldX;
	relY = posY - oldY;
}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
