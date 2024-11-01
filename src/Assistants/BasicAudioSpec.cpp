/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicAudioSpec.hpp"

/*==================================================================*/
	#pragma region BasicAudioSpec Singleton Class

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

AudioSpecBlock::AudioSpecBlock(
	const SDL_AudioFormat format, const s32 channels,
	const s32 frequency, const SDL_AudioDeviceID device
) noexcept {
	SDL_InitSubSystem(SDL_INIT_AUDIO);
	mSpec   = { format, std::max(channels, 1), std::max(frequency, 1) };
	pStream = SDL_OpenAudioDeviceStream(device, &mSpec, nullptr, nullptr);
	SDL_ResumeAudioStreamDevice(pStream);
	setVolume(255);
}

AudioSpecBlock::~AudioSpecBlock() noexcept {
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void AudioSpecBlock::setVolume(const s32 value) noexcept {
	mVolume = std::clamp(value, 0, 255);
}

void AudioSpecBlock::changeVolume(const s32 delta) noexcept {
	mVolume = std::clamp(mVolume + delta, 0, 255);
}
