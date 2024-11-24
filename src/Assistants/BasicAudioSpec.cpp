/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicAudioSpec.hpp"

/*==================================================================*/
	#pragma region BasicAudioSpec Singleton Class

BasicAudioSpec::BasicAudioSpec() noexcept {
	mSuccessful = SDL_InitSubSystem(SDL_INIT_AUDIO);
	if (!mSuccessful) {
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR, "Failed to init SDL audio!",
			SDL_GetError(), nullptr
		);
	}
}

BasicAudioSpec::~BasicAudioSpec() noexcept {
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void BasicAudioSpec::setGlobalGain(const f32 gain) noexcept {
	mGlobalGain = std::clamp(gain, 0.0f, 1.0f);
}

void BasicAudioSpec::addGlobalGain(const f32 gain) noexcept {
	mGlobalGain = std::clamp(mGlobalGain + gain, 0.0f, 1.0f);
}

void BasicAudioSpec::addGlobalGain(const s32 gain) noexcept {
	static constexpr f32 minF{ 1.0f / 255.0f };
	mGlobalGain = std::clamp(mGlobalGain + gain * minF, 0.0f, 1.0f);
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region BasicAudioSpec Singleton Class

bool AudioSpecBlock::isStreamPaused(const u32 index) const noexcept {
	assert(index < mAudioStreams.size() && "isStreamPaused() index out-of-bounds");
	const auto deviceID{ SDL_GetAudioStreamDevice(mAudioStreams[index]) };
	return deviceID ? SDL_AudioDevicePaused(deviceID) : true;
}

f32  AudioSpecBlock::getSampleRate(const f32 framerate) const noexcept {
	return framerate > 1.0f ? mAudioSpec.freq / framerate * mAudioSpec.channels : 0.0f;
}

f32  AudioSpecBlock::getGain(const u32 index) const noexcept {
	assert(index < mAudioStreams.size() && "getGain() index out-of-bounds");
	return SDL_GetAudioStreamGain(mAudioStreams[index]);
}

s32  AudioSpecBlock::getGainByte(const u32 index) const noexcept {
	assert(index < mAudioStreams.size() && "getGainByte() index out-of-bounds");
	return static_cast<s32>(getGain(index) * 255.0f);
}

/*==================================================================*/

void AudioSpecBlock::pauseStream(const u32 index) noexcept {
	assert(index < mAudioStreams.size() && "pauseStream() index out-of-bounds");
	SDL_PauseAudioStreamDevice(mAudioStreams[index]);
}

void AudioSpecBlock::pauseStreams() noexcept {
	for (const auto& stream : mAudioStreams) {
		SDL_PauseAudioStreamDevice(stream);
	}
}

void AudioSpecBlock::resumeStream(const u32 index) noexcept {
	assert(index < mAudioStreams.size() && "resumeStream() index out-of-bounds");
	SDL_ResumeAudioStreamDevice(mAudioStreams[index]);
}

void AudioSpecBlock::resumeStreams() noexcept {
	for (const auto& stream : mAudioStreams) {
		SDL_ResumeAudioStreamDevice(stream);
	}
}

void AudioSpecBlock::setGain(const u32 index, const f32 gain) noexcept {
	assert(index < mAudioStreams.size() && "setGain() index out-of-bounds");
	SDL_SetAudioStreamGain(mAudioStreams[index], gain);
}

void AudioSpecBlock::addGain(const u32 index, const f32 gain) noexcept {
	assert(index < mAudioStreams.size() && "addGain() index out-of-bounds");
	setGain(index, std::clamp(getGain(index) + gain, 0.0f, 1.0f));
}

void AudioSpecBlock::addGain(const u32 index, const s32 gain) noexcept {
	assert(index < mAudioStreams.size() && "addGain() index out-of-bounds");
	static constexpr f32 minF{ 1.0f / 255.0f };
	setGain(index, std::clamp(getGain(index) + gain * minF, 0.0f, 1.0f));
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
