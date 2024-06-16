/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL3/SDL.h>
#include <cstddef>

class BasicAudioSpec final {
public:
	static constexpr Sint32 outFrequency{ 48'000 };
	float  bytesLeft{};
	Sint16 volume{};
	Sint16 amplitude{};

private:
	SDL_AudioSpec     audiospec{};
	SDL_AudioDeviceID device{};
	SDL_AudioStream*  stream{};

public:
	explicit BasicAudioSpec();
	~BasicAudioSpec();

	void pushAudioData(const void*, std::size_t);

	void setVolume(Sint32);
	void changeVolume(Sint32);
};
