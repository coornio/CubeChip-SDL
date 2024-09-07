/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL3/SDL.h>

#include "Typedefs.hpp"

/*==================================================================*/
	#pragma region BasicAudioSpec Singleton Class
/*==================================================================*/

class BasicAudioSpec final {
	BasicAudioSpec() noexcept;
	~BasicAudioSpec() noexcept;
	BasicAudioSpec(const BasicAudioSpec&) = delete;
	BasicAudioSpec& operator=(const BasicAudioSpec&) = delete;

	static constexpr
	u32 outFrequency{ 48'000 };

	s16 volume{};
	s16 amplitude{};

	SDL_AudioSpec     audiospec{};
	SDL_AudioDeviceID device{};
	SDL_AudioStream*  stream{};

	static bool& errorState() noexcept {
		static bool errorEncountered{};
		return errorEncountered;
	}

public:
	static auto* create() noexcept {
		static BasicAudioSpec self;
		return errorState() ? nullptr : &self;
	}

	static void setErrorState(const bool state) noexcept { errorState() = state; }
	static bool getErrorState()                 noexcept { return errorState();  }
	
	void pushAudioData(const void*, usz);

	s32  getFrequency()  const noexcept { return outFrequency; }
	s16  getAmplitude()  const noexcept { return amplitude; }
	s16  getVolume()     const noexcept { return volume; }
	f32  getVolumeNorm() const noexcept { return volume / 255.0f; }

	void setVolume(s32) noexcept;
	void changeVolume(s32) noexcept;
};

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
