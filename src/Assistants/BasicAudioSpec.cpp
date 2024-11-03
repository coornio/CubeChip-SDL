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
	const s32 frequency, const s32 streams, const SDL_AudioDeviceID device
) noexcept
	: mAudioStreams(std::max(streams, 1))
{
	SDL_InitSubSystem(SDL_INIT_AUDIO);
	mAudioSpec = { format, std::max(channels, 1), std::max(frequency, 1) };
	for (auto& stream : mAudioStreams) {
		stream.ptr = SDL_OpenAudioDeviceStream(device, &mAudioSpec, nullptr, nullptr);
		SDL_ResumeAudioStreamDevice(stream.ptr);
	}
}

AudioSpecBlock::~AudioSpecBlock() noexcept {
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void AudioSpecBlock::setVolume(const u32 index, const s32 value) noexcept {
	mAudioStreams[index].volume = std::clamp(value, 0, 255);
}

void AudioSpecBlock::changeVolume(const u32 index, const s32 delta) noexcept {
	mAudioStreams[index].volume = std::clamp(mAudioStreams[index].volume + delta, 0, 255);
}
