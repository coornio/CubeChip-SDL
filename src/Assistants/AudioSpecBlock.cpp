/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <vector>
#include <algorithm>
#include <cassert>

#include "BasicAudioSpec.hpp"
#include "AudioSpecBlock.hpp"
#include "LifetimeWrapperSDL.hpp"

#include <SDL3/SDL_audio.h>

/*==================================================================*/

static SDL_AudioFormat parseFormat(AUDIOFORMAT format) noexcept {
	switch (format) {
		case AUDIOFORMAT::U8:
			return SDL_AUDIO_U8;

		case AUDIOFORMAT::S8:
			return SDL_AUDIO_S8;

		case AUDIOFORMAT::S16LE:
			return SDL_AUDIO_S16LE;

		case AUDIOFORMAT::S16BE:
			return SDL_AUDIO_S16BE;

		case AUDIOFORMAT::S32LE:
			return SDL_AUDIO_S32LE;

		case AUDIOFORMAT::S32BE:
			return SDL_AUDIO_S32BE;

		case AUDIOFORMAT::F32LE:
			return SDL_AUDIO_F32LE;

		case AUDIOFORMAT::F32BE:
			return SDL_AUDIO_F32BE;

		case AUDIOFORMAT::UNKNOWN:
		default: return SDL_AUDIO_UNKNOWN;
	}
}

static float calculateGain(float streamGain) noexcept {
	return streamGain * (BasicAudioSpec::isMuted()
		? 0.0f : BasicAudioSpec::getGlobalGain());
}

/*==================================================================*/

struct AudioSpecBlock::Internals {
	struct Stream {
		SDL_Unique<SDL_AudioStream> ptr;
		float gain{};

		operator SDL_AudioStream*() const noexcept
			{ return ptr.get(); }
	};

	const SDL_AudioSpec audioSpec;
	std::vector<Stream> audioStreams;

	Internals(
		unsigned streams, signed channels, signed frequency,
		SDL_AudioFormat format, SDL_AudioDeviceID device
	) noexcept
		: audioSpec{ format, channels, frequency }
		, audioStreams(streams)
	{
		for (auto& audioStream : audioStreams) {
			audioStream.ptr  = SDL_OpenAudioDeviceStream(device, &audioSpec, nullptr, nullptr);
			audioStream.gain = 1.0f;
		}
	}
};

/*==================================================================*/
	#pragma region AudioSpecBlock Class

AudioSpecBlock::AudioSpecBlock(
	AUDIOFORMAT format,
	unsigned channels, unsigned frequency,
	unsigned streams,  unsigned device
) noexcept
	: mSDL{ std::make_unique<Internals>(
		streams, channels, frequency, parseFormat(format),
		device ? device : SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK
	) }
{}

AudioSpecBlock::~AudioSpecBlock() noexcept {}

/*==================================================================*/

signed AudioSpecBlock::getFrequency() const noexcept {
	return mSDL->audioSpec.freq;
}

signed AudioSpecBlock::getStreamCount() const noexcept {
	return static_cast<signed>(mSDL->audioStreams.size());
}

bool AudioSpecBlock::isStreamPaused(unsigned index) const noexcept {
	assert(index < mSDL->audioStreams.size() && "isStreamPaused() index out-of-bounds");
	const auto deviceID{ SDL_GetAudioStreamDevice(mSDL->audioStreams[index]) };
	return deviceID ? SDL_AudioDevicePaused(deviceID) : true;
}

float AudioSpecBlock::getSampleRate(float framerate) const noexcept {
	return framerate > 1.0f ? mSDL->audioSpec.freq / framerate * mSDL->audioSpec.channels : 0.0f;
}

float AudioSpecBlock::getGain(unsigned index) const noexcept {
	assert(index < mSDL->audioStreams.size() && "getGain() index out-of-bounds");
	return mSDL->audioStreams[index].gain;
}

signed AudioSpecBlock::getGainByte(unsigned index) const noexcept {
	assert(index < mSDL->audioStreams.size() && "getGainByte() index out-of-bounds");
	return static_cast<signed>(getGain(index) * 255.0f);
}

/*==================================================================*/

void AudioSpecBlock::pauseStream(unsigned index) noexcept {
	assert(index < mSDL->audioStreams.size() && "pauseStream() index out-of-bounds");
	SDL_PauseAudioStreamDevice(mSDL->audioStreams[index]);
}

void AudioSpecBlock::pauseStreams() noexcept {
	for (auto& stream : mSDL->audioStreams)
		{ SDL_PauseAudioStreamDevice(stream); }
}

void AudioSpecBlock::resumeStream(unsigned index) noexcept {
	assert(index < mSDL->audioStreams.size() && "resumeStream() index out-of-bounds");
	SDL_ResumeAudioStreamDevice(mSDL->audioStreams[index]);
}

void AudioSpecBlock::resumeStreams() noexcept {
	for (auto& stream : mSDL->audioStreams)
		{ SDL_ResumeAudioStreamDevice(stream); }
}

void AudioSpecBlock::setGain(unsigned index, float gain) noexcept {
	assert(index < mSDL->audioStreams.size() && "setGain() index out-of-bounds");
	mSDL->audioStreams[index].gain = std::clamp(gain, 0.0f, 2.0f);
}

void AudioSpecBlock::addGain(unsigned index, float gain) noexcept {
	assert(index < mSDL->audioStreams.size() && "addGain() index out-of-bounds");
	setGain(index, getGain(index) + gain);
}

void AudioSpecBlock::addGain(unsigned index, signed gain) noexcept {
	assert(index < mSDL->audioStreams.size() && "addGain() index out-of-bounds");
	setGain(index, getGain(index) + gain * (1.0f / 255.0f));
}

void AudioSpecBlock::pushRawAudio(
	unsigned index, void* sampleData,
	std::size_t bufferSize, std::size_t sampleSize
) const {
	if (isStreamPaused(index) || bufferSize == 0u) { return; }

	SDL_SetAudioStreamGain(mSDL->audioStreams[index],
		calculateGain(mSDL->audioStreams[index].gain));

	SDL_PutAudioStreamData(
		mSDL->audioStreams[index], sampleData,
		static_cast<signed>(bufferSize * sampleSize)
	);
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
