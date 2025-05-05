/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <algorithm>

#include "BasicInput.hpp"

/*==================================================================*/

#ifdef __APPLE__
	#define EXEC_POLICY(policy)
#else
	#include <execution>
	#define EXEC_POLICY(policy) std::execution::policy,
#endif

/*==================================================================*/

void BasicKeyboard::updateStates() noexcept {
	std::copy_n(EXEC_POLICY(unseq)
		mCurState, TOTALKEYS, mOldState);

	std::copy_n(EXEC_POLICY(unseq)
		SDL_GetKeyboardState(nullptr), TOTALKEYS, mCurState);
}

void BasicMouse::updateStates() noexcept {
	mOldState = mCurState;

	const auto oldX{ mPosX }, oldY{ mPosY };
	mCurState = SDL_GetMouseState(&mPosX, &mPosY);
	mRelX = mPosX - oldX; mRelY = mPosY - oldY;
}
