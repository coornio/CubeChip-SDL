/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cmath>
#include <atomic>
#include <vector>
#include <memory>
#include <cassert>
#include <utility>
#include <algorithm>

#include <SDL3/SDL.h>

#include "Typedefs.hpp"
#include "Concepts.hpp"
#include "LifetimeWrapperSDL.hpp"

/*==================================================================*/
	#pragma region BasicAudioSpec Singleton Class

class BasicAudioSpec final {

public:
	struct Settings {
		f32  globalGain{};
		bool isMuted{};
	};

	[[nodiscard]]
	auto exportSettings() const noexcept {
		Settings out;

		out.globalGain = mGlobalGain.load(mo::relaxed);
		out.isMuted    = mIsMuted.load(mo::relaxed);

		return out;
	}

private:
	BasicAudioSpec(const Settings& settings) noexcept;
	~BasicAudioSpec() noexcept;
	BasicAudioSpec(const BasicAudioSpec&) = delete;
	BasicAudioSpec& operator=(const BasicAudioSpec&) = delete;

	static inline Atom<f32> mGlobalGain{};
	static inline Atom<bool> mIsMuted{};
	static inline bool mSuccessful{ true };

public:
	static auto* create(const Settings& settings) noexcept {
		static BasicAudioSpec self(settings);
		return mSuccessful ? &self : nullptr;
	}

	static bool isSuccessful() noexcept { return mSuccessful; }

	static bool isMuted()           noexcept { return mIsMuted.load(mo::relaxed); }
	static void isMuted(bool state) noexcept { mIsMuted.store(state, mo::relaxed); }
	static void toggleMuted()       noexcept { mIsMuted.store(isMuted(), mo::relaxed); }

	static auto getGlobalGain()     noexcept { return mGlobalGain.load(mo::relaxed); }
	static auto getGlobalGainByte() noexcept { return static_cast<s32>(getGlobalGain() * 255.0f); }

	static void setGlobalGain(f32 gain) noexcept;
	static void addGlobalGain(f32 gain) noexcept;
	static void addGlobalGain(s32 gain) noexcept;
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

class AudioSpecBlock {
	SDL_AudioSpec mAudioSpec;
	std::vector<SDL_Unique<SDL_AudioStream>>
		mAudioStreams;

public:
	AudioSpecBlock(
		SDL_AudioFormat format, s32 channels, s32 frequency, s32 streams,
		SDL_AudioDeviceID device = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK
	) noexcept;

	AudioSpecBlock(const AudioSpecBlock&) = delete;
	AudioSpecBlock& operator=(const AudioSpecBlock&) = delete;

	/*==================================================================*/

	auto getFrequency()   const noexcept { return mAudioSpec.freq; }
	auto getStreamCount() const noexcept { return static_cast<s32>(mAudioStreams.size()); }

	bool isStreamPaused(u32 index)    const noexcept;
	f32  getSampleRate(f32 framerate) const noexcept;
	f32  getGain(u32 index)           const noexcept;
	s32  getGainByte(u32 index)       const noexcept;

	/*==================================================================*/

	void pauseStream(u32 index) noexcept;
	void pauseStreams() noexcept;

	void resumeStream(u32 index) noexcept;
	void resumeStreams() noexcept;

	void setGain(u32 index, f32 gain) noexcept;
	void addGain(u32 index, f32 gain) noexcept;
	void addGain(u32 index, s32 gain) noexcept;

	/**
	 * @brief Pushes buffer of audio samples to SDL device/stream.
	 * @param[in] index :: the device/stream to push audio to.
	 * @param[in] sampleData :: pointer to audio samples buffer.
	 * @param[in] bufferSize :: size of buffer in bytes.
	 */
	template <typename T> requires (std::is_arithmetic_v<T>)
	void pushAudioData(u32 index, T* sampleData, ust bufferSize) const {
		if (isStreamPaused(index) || !bufferSize) { return; }

		const auto globalGain{ BasicAudioSpec::isMuted()
			? 0.0f : BasicAudioSpec::getGlobalGain() };

		std::transform(EXEC_POLICY(unseq)
			sampleData, sampleData + bufferSize, sampleData,
			[globalGain](const T sample) noexcept {
				return static_cast<T>(sample * globalGain);
			}
		);

		SDL_PutAudioStreamData(
			mAudioStreams[index], sampleData,
			static_cast<s32>(bufferSize * sizeof(T))
		);
	}

	/**
	 * @brief Pushes buffer of audio samples to SDL device/stream.
	 * @param[in] index :: the device/stream to push audio to.
	 * @param[in] samplesBuffer :: audio samples buffer (C style).
	 */
	template <typename T, size_type N> requires (std::is_arithmetic_v<T>)
	void pushAudioData(u32 index, T(&samplesBuffer)[N]) const {
		pushAudioData(index, samplesBuffer, N);
	}

	/**
	 * @brief Pushes buffer of audio samples to SDL device/stream.
	 * @param[in] index :: the device/stream to push audio to.
	 * @param[in] samplesBuffer :: audio samples buffer (C++ style).
	 */
	template <IsContiguousContainer T>
	void pushAudioData(u32 index, T& samplesBuffer) const {
		pushAudioData(index, std::data(samplesBuffer), std::size(samplesBuffer));
	}
};
