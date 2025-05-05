/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicAudioSpec.hpp"

#include <algorithm>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>

/*==================================================================*/
	#pragma region BasicAudioSpec Singleton Class

BasicAudioSpec::BasicAudioSpec(const Settings& settings) noexcept {
	mSuccessful = SDL_InitSubSystem(SDL_INIT_AUDIO);
	if (!mSuccessful) {
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR, "Failed to init Audio Subsystem!",
			SDL_GetError(), nullptr
		);
		return;
	}

	setGlobalGain(settings.volume);
	isMuted(settings.muted);
}

BasicAudioSpec::~BasicAudioSpec() noexcept {
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void BasicAudioSpec::setGlobalGain(float gain) noexcept {
	mGlobalGain.store(std::clamp(gain, 0.0f, 1.0f), mo::relaxed);
}

void BasicAudioSpec::addGlobalGain(float gain) noexcept {
	mGlobalGain.store(std::clamp(getGlobalGain() + gain, 0.0f, 1.0f), mo::relaxed);
}

void BasicAudioSpec::addGlobalGain(signed gain) noexcept {
	static constexpr float minF{ 1.0f / 255.0f };
	mGlobalGain.store(std::clamp(getGlobalGain() + gain * minF, 0.0f, 1.0f), mo::relaxed);
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
