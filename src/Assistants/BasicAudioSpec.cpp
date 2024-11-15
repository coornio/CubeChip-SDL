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

s32 AudioSpecBlock::getFrequency() const noexcept {
	return mAudioSpec.freq;
}

s32 AudioSpecBlock::getStreamCount() const noexcept {
	return static_cast<s32>(mAudioStreams.size());
}

bool AudioSpecBlock::isPaused(const u32 index) const noexcept {
	const auto deviceID{ SDL_GetAudioStreamDevice(mAudioStreams[index]) };
	return deviceID ? SDL_AudioDevicePaused(deviceID) : true;
}

f32  AudioSpecBlock::getSampleRate(const f32 framerate) const noexcept {
	return framerate > Epsilon::f32 ? mAudioSpec.freq / framerate : 0.0f;
}

f32  AudioSpecBlock::getGain(const u32 index) const noexcept {
	return SDL_GetAudioStreamGain(mAudioStreams[index]);
}

s32 AudioSpecBlock::getGainByte(const u32 index) const noexcept {
	return static_cast<s32>(getGain(index) * 255.0f);
}

void AudioSpecBlock::pauseStream(const u32 index) noexcept {
	SDL_PauseAudioStreamDevice(mAudioStreams[index]);
}

void AudioSpecBlock::resumeStream(const u32 index) noexcept {
	SDL_ResumeAudioStreamDevice(mAudioStreams[index]);
}

void AudioSpecBlock::setGain(const u32 index, const f32 gain) noexcept {
	SDL_SetAudioStreamGain(mAudioStreams[index], gain);
}

void AudioSpecBlock::addGain(const u32 index, const s32 gain) noexcept {
	static constexpr f32 minF{ 1.0f / 255.0f };
	setGain(index, std::clamp(getGain(index) + gain * minF, 0.0f, 1.0f));
}

void AudioSpecBlock::addGain(const u32 index, const f32 gain) noexcept {
	setGain(index, std::clamp(getGain(index) + gain, 0.0f, 1.0f));
}
