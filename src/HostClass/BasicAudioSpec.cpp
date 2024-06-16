/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicAudioSpec.hpp"
#include <algorithm>

static constexpr Sint32 VOL_MAX{ 255 };
static constexpr Sint32 VOL_MIN{ 0 };

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

void BasicAudioSpec::pushAudioData(const void* const data, const std::size_t len) {
	SDL_PutAudioStreamData(stream, data, static_cast<Sint32>(len * 2));
}

void BasicAudioSpec::setVolume(const Sint32 vol) {
	volume    = static_cast<Sint16>(std::clamp(std::abs(vol), VOL_MIN, VOL_MAX));
	amplitude = static_cast<Sint16>(16 * volume);
}

void BasicAudioSpec::changeVolume(const Sint32 delta) {
	volume    = static_cast<Sint16>(std::clamp(volume + delta, VOL_MIN, VOL_MAX));
	amplitude = static_cast<Sint16>(16 * volume);
}
