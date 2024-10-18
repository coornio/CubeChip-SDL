/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <span>
#include <cmath>
#include <vector>
#include <utility>
#include <algorithm>
#include <execution>

#include <SDL3/SDL.h>

#include "Typedefs.hpp"

/*==================================================================*/
	#pragma region BasicAudioSpec Singleton Class

class BasicAudioSpec final {
	BasicAudioSpec() = default;
	~BasicAudioSpec() = default;
	BasicAudioSpec(const BasicAudioSpec&) = delete;
	BasicAudioSpec& operator=(const BasicAudioSpec&) = delete;

	static bool& errorState() noexcept {
		static bool errorEncountered{};
		return errorEncountered;
	}

	static s32& globalVolume() noexcept {
		static s32 globalVolume{ 15 * 11 };
		return globalVolume;
	}

public:
	static auto* create() noexcept {
		static BasicAudioSpec self;
		return errorState() ? nullptr : &self;
	}

	static bool getErrorState()                 noexcept { return errorState();  }
	static void setErrorState(const bool state) noexcept { errorState() = state; }

	static auto getGlobalVolume()     noexcept { return globalVolume(); }
	static auto getGlobalVolumeNorm() noexcept { return globalVolume() / 255.0f; }

	static void setGlobalVolume(s32 value) noexcept {
		auto& volume{ globalVolume() };
		volume = std::clamp(value, 0, 255);
	}
	static void changeGlobalVolume(s32 delta) noexcept {
		auto& volume{ globalVolume() };
		volume = std::clamp(volume + delta, 0, 255);
	}
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

#include <iostream>

class AudioSpecBlock {
	SDL_AudioSpec     mSpec;
	SDL_AudioDeviceID mDevice{};
	SDL_AudioStream*  pStream{};

	s32 mVolume{};

public:
	AudioSpecBlock(
		const SDL_AudioFormat format, const s32 channels, const s32 frequency,
		const SDL_AudioDeviceID device = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK
	) noexcept {
		SDL_InitSubSystem(SDL_INIT_AUDIO);
		mSpec   = { format, std::max(channels, 1), std::max(frequency, 1) };
		pStream = SDL_OpenAudioDeviceStream(device, &mSpec, nullptr, nullptr);
		mDevice = SDL_GetAudioStreamDevice(pStream);
		SDL_ResumeAudioStreamDevice(pStream);
		setVolume(255);
	}
	~AudioSpecBlock() noexcept {
		//SDL_DestroyAudioStream(pStream);
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}

	AudioSpecBlock(const AudioSpecBlock&) = delete;
	AudioSpecBlock& operator=(const AudioSpecBlock&) = delete;

	auto getFrequency()  const noexcept { return mSpec.freq; }
	auto getVolume()     const noexcept { return mVolume; }
	auto getVolumeNorm() const noexcept { return mVolume / 255.0f; }

	auto getSampleRate(f32 framerate) const noexcept {
		return std::abs(framerate) > 1e-6f ? mSpec.freq / framerate : 0.0f;
	}

	void setVolume(s32 value) noexcept {
		mVolume = std::clamp(value, 0, 255);
	}
	void changeVolume(s32 delta) noexcept {
		mVolume = std::clamp(mVolume + delta, 0, 255);
	}

	/**
	 * @brief Pushes buffer of audio samples to SDL, accepts any span
	 * and implicitly converts type to appropriate byte count.
	 * @param[in] samplesBuffer :: Span to array of audio samples.
	 */
	template <typename T>
	void pushAudioData(const std::span<T> samplesBuffer) const {
		if (!pStream || !mDevice) { return; }

		const auto localVolume { AudioSpecBlock::getVolumeNorm() };
		const auto globalVolume{ BasicAudioSpec::getGlobalVolumeNorm() };

		std::transform(
			std::execution::unseq,
			samplesBuffer.begin(),
			samplesBuffer.end(),
			samplesBuffer.data(),
			[localVolume, globalVolume](const T sample) noexcept {
				return static_cast<T>(sample * localVolume * globalVolume);
			}
		);

		SDL_PutAudioStreamData(pStream, samplesBuffer.data(),
			static_cast<s32>(sizeof(T) * samplesBuffer.size())
		);
	}
};

