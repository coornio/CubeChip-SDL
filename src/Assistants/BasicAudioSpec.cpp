/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicAudioSpec.hpp"

/*==================================================================*/
	#pragma region BasicAudioSpec Singleton Class

BasicAudioSpec::BasicAudioSpec() noexcept {
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		showErrorBox("Failed to init SDL audio!");
		return;
	}
}

BasicAudioSpec::~BasicAudioSpec() noexcept {
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void BasicAudioSpec::showErrorBox(const char* const title) noexcept {
	setErrorState(true);
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title,
		SDL_GetError(), nullptr
	);
}

void BasicAudioSpec::setGlobalVolume(s32 value) noexcept {
	auto& volume{ globalVolume() };
	volume = std::clamp(value, 0, 255);
}

void BasicAudioSpec::changeGlobalVolume(s32 delta) noexcept {
	auto& volume{ globalVolume() };
	volume = std::clamp(volume + delta, 0, 255);
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
