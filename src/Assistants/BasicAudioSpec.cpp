/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicAudioSpec.hpp"
#include <SDL3/SDL_audio.h>
#include <algorithm>

static constexpr s32 VOL_MAX{ 255 };
static constexpr s32 VOL_MIN{   0 };

/*==================================================================*/
	#pragma region BasicAudioSpec Singleton Class
/*==================================================================*/

BasicAudioSpec::BasicAudioSpec() {
	//SDL_InitSubSystem(SDL_INIT_AUDIO);
}

BasicAudioSpec::~BasicAudioSpec() {
	//SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
