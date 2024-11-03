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
#include <concepts>
#include <algorithm>
#include <execution>

#include <SDL3/SDL.h>

#include "Typedefs.hpp"
#include "LifetimeWrapperSDL.hpp"

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

	static void setGlobalVolume(s32 value) noexcept;
	static void changeGlobalVolume(s32 delta) noexcept;
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

class AudioSpecBlock {
	
	struct StreamBlock {
		using Stream = SDL_AudioStream;
		s32 volume{ 255 };
		Stream* ptr{};
	};

	SDL_AudioSpec mAudioSpec;
	std::vector<StreamBlock>
		mAudioStreams;

public:
	AudioSpecBlock(
		const SDL_AudioFormat format, const s32 channels, const s32 frequency, const s32 streams,
		const SDL_AudioDeviceID device = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK
	) noexcept;
	~AudioSpecBlock() noexcept;

	AudioSpecBlock(const AudioSpecBlock&) = delete;
	AudioSpecBlock& operator=(const AudioSpecBlock&) = delete;

	auto getFrequency()                 const noexcept { return mAudioSpec.freq; }
	auto getVolume(const u32 index)     const noexcept { return mAudioStreams[index].volume; }
	auto getVolumeNorm(const u32 index) const noexcept { return mAudioStreams[index].volume / 255.0f; }

	auto getSampleRate(const f32 framerate) const noexcept {
		return framerate > Epsilon::f32 ? mAudioSpec.freq / framerate : 0.0f;
	}

	void setVolume(const u32 index, const s32 value) noexcept;
	void changeVolume(const u32 index, const s32 delta) noexcept;

	/**
	 * @brief Pushes buffer of audio samples to SDL, accepts any span
	 * and implicitly converts type to appropriate byte count.
	 * @param[in] samplesBuffer :: Span to array of audio samples.
	 */
	template <typename T>
	void pushAudioData(const u32 index, const std::span<T> samplesBuffer) const {
		if (!mAudioStreams[index].ptr) { return; }

		const auto localVolume { AudioSpecBlock::getVolumeNorm(index) };
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

		SDL_PutAudioStreamData(mAudioStreams[index].ptr, samplesBuffer.data(),
			static_cast<s32>(sizeof(T) * samplesBuffer.size())
		);
	}
};

