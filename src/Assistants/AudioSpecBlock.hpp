/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <memory>

#include "Concepts.hpp"
#include "LifetimeWrapperSDL.hpp"


/*==================================================================*/

enum class AUDIOFORMAT : unsigned {
	UNKNOWN, U8, S8,
	S16LE, S16BE,
	S32LE, S32BE,
	F32LE, F32BE,
	S16 = S16LE,
	S32 = S32LE,
	F32 = F32LE,
};

struct SDL_AudioSpec;

/*==================================================================*/

class AudioSpecBlock {
	using self = AudioSpecBlock;

	class Stream {
		SDL_Unique<SDL_AudioStream> ptr;
		signed format{}; signed freq{};
		signed channels{}; float gain{ 1.0f };
		unsigned long long accumulator{};

	public:
		Stream(
			SDL_AudioStream* ptr,
			signed format, signed freq, signed channels
		) noexcept;

		auto getSpec() const noexcept -> SDL_AudioSpec;

		auto getFormat()   const noexcept { return format; }
		auto getFreq()     const noexcept { return freq; }
		auto getChannels() const noexcept { return channels; }

		bool isPaused()   const noexcept;
		bool isPlayback() const noexcept;

		double getRawSampleRate(double framerate) const noexcept;

		[[nodiscard]]
		unsigned getNextBufferSize(double framerate) noexcept;

		void pause() noexcept;
		void resume() noexcept;

		float getGain() const noexcept;
		void  setGain(float gain) noexcept;
		void  addGain(float gain) noexcept;

		operator SDL_AudioStream*() const noexcept
			{ return ptr.get(); }

		void pushRawAudio(void* sampleData, std::size_t bufferSize, std::size_t sampleSize) const;

		/**
		 * @brief Pushes buffer of audio samples to SDL device/stream.
		 * @param[in] index :: the device/stream to push audio to.
		 * @param[in] sampleData :: pointer to audio samples buffer.
		 * @param[in] bufferSize :: size of buffer in bytes.
		 */
		template <IsPlainOldData T>
		void pushAudioData(T* sampleData, std::size_t bufferSize) const
			{ pushRawAudio(sampleData, bufferSize, sizeof(T)); }

		/**
		 * @brief Pushes buffer of audio samples to SDL device/stream.
		 * @param[in] index :: the device/stream to push audio to.
		 * @param[in] samplesBuffer :: audio samples buffer (C style).
		 */
		template <IsPlainOldData T, std::size_t N>
		void pushAudioData(T(&samplesBuffer)[N]) const
			{ pushRawAudio(samplesBuffer, N, sizeof(T)); }

		/**
		 * @brief Pushes buffer of audio samples to SDL device/stream.
		 * @param[in] index :: the device/stream to push audio to.
		 * @param[in] samplesBuffer :: audio samples buffer (C++ style).
		 */
		template <IsContiguousContainer T> requires(IsPlainOldData<ValueType<T>>)
		void pushAudioData(T& samplesBuffer) const
			{ pushRawAudio(std::data(samplesBuffer), std::size(samplesBuffer), sizeof(ValueType<T>)); }
	};

	std::unordered_map<signed, Stream> audioStreams{};

public:
	//using format = std::conditional_t<
	//	T == SDL_AUDIO_U8, u8, std::conditional_t<
	//	T == SDL_AUDIO_S8, s8, std::conditional_t<
	//	(T == SDL_AUDIO_S16LE || T == SDL_AUDIO_S16BE), s16, std::conditional_t<
	//	(T == SDL_AUDIO_S32LE || T == SDL_AUDIO_S32BE), s32, std::conditional_t<
	//	(T == SDL_AUDIO_F32LE || T == SDL_AUDIO_F32BE), f32, void // fallback (should never reach)
	//>>>>>;

public:
	AudioSpecBlock() noexcept = default;
	~AudioSpecBlock() noexcept;

	AudioSpecBlock(const self&)  = delete;
	self& operator=(const self&) = delete;

	bool addAudioStream(
		signed streamID, AUDIOFORMAT format,
		signed channels, signed frequency, unsigned device = 0
	);

	unsigned getStreamCount() const noexcept;
	void pauseStreams()  noexcept;
	void resumeStreams() noexcept;

	[[nodiscard]]
	Stream& operator[](signed key) noexcept
		{ return audioStreams.at(key); }

	[[nodiscard]]
	Stream* at(signed key) noexcept {
		return audioStreams.contains(key)
			? &audioStreams.at(key) : nullptr;
	}
};
