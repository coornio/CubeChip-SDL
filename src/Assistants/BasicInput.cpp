/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicInput.hpp"
#include <execution>

/*------------------------------------------------------------------*/
/*  class  BasicInput.hpp > Keyboard                                */
/*------------------------------------------------------------------*/

BasicKeyboard& BasicKeyboard::create() {
	if (!_self) { _self = std::make_unique<BasicKeyboard>(); }
	return *_self.get();
}

std::unique_ptr<BasicKeyboard> BasicKeyboard::_self{ nullptr };
BasicKeyboard& bic::kb{ BasicKeyboard::create() };

void BasicKeyboard::updateCopy() {
	std::copy_n(
		std::execution::par_unseq,
		SDL_GetKeyboardState(nullptr),
		SDL_NUM_SCANCODES,
		oldState
	);
}

/*------------------------------------------------------------------*/
/*  class  BasicInput.hpp > Mouse                                   */
/*------------------------------------------------------------------*/

BasicMouse& BasicMouse::create() {
	if (!_self) { _self = std::make_unique<BasicMouse>(); }
	return *_self.get();
}

std::unique_ptr<BasicMouse> BasicMouse::_self{ nullptr };
BasicMouse& bic::mb{ BasicMouse::create() };

void BasicMouse::updateCopy() {
	const auto oldX{ posX };
	const auto oldY{ posY };
	oldState = SDL_GetMouseState(&posX, &posY);
	relX = posX - oldX;
	relY = posY - oldY;
}
