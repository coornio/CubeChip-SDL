/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicAudioSpec.hpp"
#include <algorithm>

static constexpr s32 VOL_MAX{ 255 };
static constexpr s32 VOL_MIN{   0 };

static constexpr float VOL_FMAX{ 255.0f };
static constexpr float VOL_FMIN{   0.0f };

BasicAudioSpec::BasicAudioSpec()
	: audiospec{ SDL_AUDIO_S16, 1, outFrequency }
{
	SDL_InitSubSystem(SDL_INIT_AUDIO);
	setVolume(VOL_MAX);

	stream = SDL_OpenAudioDeviceStream(
		SDL_AUDIO_DEVICE_DEFAULT_OUTPUT,
		&audiospec, nullptr, nullptr
	);
	device = SDL_GetAudioStreamDevice(stream);
	SDL_ResumeAudioDevice(device);
}

BasicAudioSpec::~BasicAudioSpec() {
	if (stream) SDL_DestroyAudioStream(stream);
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void BasicAudioSpec::pushAudioData(const void* const data, const usz length) {
	SDL_PutAudioStreamData(stream, data, static_cast<s32>(length * 2));
}

s32   BasicAudioSpec::getFrequency()  const { return outFrequency; }
s16   BasicAudioSpec::getAmplitude()  const { return amplitude; }
s16   BasicAudioSpec::getVolume()     const { return volume; }
float BasicAudioSpec::getVolumeNorm() const { return volume / VOL_FMAX; }

void BasicAudioSpec::setVolume(const s32 value) {
	volume    = static_cast<s16>(std::clamp(value, VOL_MIN, VOL_MAX));
	amplitude = static_cast<s16>(16 * volume);
}

void BasicAudioSpec::changeVolume(const s32 delta) {
	volume    = static_cast<s16>(std::clamp(volume + delta, VOL_MIN, VOL_MAX));
	amplitude = static_cast<s16>(16 * volume);
}
