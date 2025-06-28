/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <vector>
#include <algorithm>
#include <cassert>

#include "Misc.hpp"
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
	#pragma region AudioSpecBlock Class

AudioSpecBlock::~AudioSpecBlock() noexcept {}

bool AudioSpecBlock::addAudioStream(
	signed streamKey, AUDIOFORMAT format,
	signed channels, signed frequency, unsigned device
) {
	SDL_AudioSpec spec{ parseFormat(format), channels, frequency };
	if (spec.format == SDL_AUDIO_UNKNOWN) { return false; }

	auto* ptr{ SDL_OpenAudioDeviceStream(
		device ? device : SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK
		, &spec, nullptr, nullptr) };

	if (audioStreams.contains(streamKey)) {
		audioStreams.at(streamKey) =
			Stream(ptr, spec.format, spec.freq, spec.channels);
	} else {
		audioStreams.emplace(streamKey,
			Stream(ptr, spec.format, spec.freq, spec.channels));
	}

	return ptr != nullptr;
}

/*==================================================================*/

unsigned AudioSpecBlock::getStreamCount() const noexcept
	{ return static_cast<unsigned>(audioStreams.size()); }

void AudioSpecBlock::pauseStreams() noexcept {
	for (auto& stream : audioStreams)
		{ SDL_PauseAudioStreamDevice(stream.second); }
}

void AudioSpecBlock::resumeStreams() noexcept {
	for (auto& stream : audioStreams)
		{ SDL_ResumeAudioStreamDevice(stream.second); }
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

AudioSpecBlock::Stream::Stream(
	SDL_AudioStream* ptr,
	signed format, signed freq, signed channels
) noexcept
	: ptr     { ptr      }
	, format  { format   }
	, freq    { freq     }
	, channels{ channels }
{}

auto AudioSpecBlock::Stream::getSpec() const noexcept -> SDL_AudioSpec {
	return { static_cast<SDL_AudioFormat>(format), freq, channels };
}

bool AudioSpecBlock::Stream::isPaused() const noexcept {
	const auto deviceID{ SDL_GetAudioStreamDevice(ptr) };
	return deviceID ? SDL_AudioDevicePaused(deviceID) : true;
}

bool AudioSpecBlock::Stream::isPlayback() const noexcept {
	return SDL_IsAudioDevicePlayback(SDL_GetAudioStreamDevice(ptr));
}

double AudioSpecBlock::Stream::getRawSampleRate(double framerate) const noexcept {
	if (framerate < 1.0) { return 0.0; }
	return freq / framerate * channels;
}

unsigned AudioSpecBlock::Stream::getNextBufferSize(double framerate) noexcept {
	if (framerate < 1.0f) { return 0u; }

	static constexpr auto scale_factor{ 1ull << 24 };
	::assign_cast(accumulator, accumulator + getRawSampleRate(framerate) * scale_factor);
	const auto sample_amount{ accumulator >> 24 };
	::assign_cast(accumulator, accumulator & (scale_factor - 1));

	return static_cast<unsigned>(sample_amount);
}

void AudioSpecBlock::Stream::pause() noexcept
	{ SDL_PauseAudioStreamDevice(ptr); }

void AudioSpecBlock::Stream::resume() noexcept
	{ SDL_ResumeAudioStreamDevice(ptr); }

float AudioSpecBlock::Stream::getGain() const noexcept
	{ return gain; }

void AudioSpecBlock::Stream::setGain(float new_gain) noexcept
	{ gain = std::clamp(new_gain, 0.0f, 2.0f); }

void AudioSpecBlock::Stream::addGain(float add_gain) noexcept
	{ setGain(gain + add_gain); }

void AudioSpecBlock::Stream::pushRawAudio(void* sampleData,
	std::size_t bufferSize, std::size_t sampleSize
) const {
	if (isPaused() || bufferSize == 0u) { return; }

	SDL_SetAudioStreamGain(ptr, calculateGain(gain));

	SDL_PutAudioStreamData(ptr, sampleData,
		static_cast<signed>(bufferSize * sampleSize));
}
