/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <memory>

#include "Concepts.hpp"

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

/*==================================================================*/

class AudioSpecBlock {

protected:
	struct Internals;
	std::unique_ptr<Internals>
		mSDL;

public:
	AudioSpecBlock(
		AUDIOFORMAT format,
		unsigned channels, unsigned frequency,
		unsigned streams,  unsigned device = 0
	) noexcept;
	~AudioSpecBlock() noexcept;

	AudioSpecBlock(const AudioSpecBlock&) = delete;
	AudioSpecBlock& operator=(const AudioSpecBlock&) = delete;

	/*==================================================================*/

	signed getFrequency()   const noexcept;
	signed getStreamCount() const noexcept;

	bool   isStreamPaused(unsigned index)     const noexcept;
	float  getSampleRate (float    framerate) const noexcept;
	float  getGain       (unsigned index)     const noexcept;
	signed getGainByte   (unsigned index)     const noexcept;

	/*==================================================================*/

	void pauseStream(unsigned index) noexcept;
	void pauseStreams() noexcept;

	void resumeStream(unsigned index) noexcept;
	void resumeStreams() noexcept;

	void setGain(unsigned index, float  gain) noexcept;
	void addGain(unsigned index, float  gain) noexcept;
	void addGain(unsigned index, signed gain) noexcept;

private:
	void pushRawAudio(
		unsigned index, void* sampleData,
		std::size_t bufferSize, std::size_t sampleSize
	) const;

public:
	/**
	 * @brief Pushes buffer of audio samples to SDL device/stream.
	 * @param[in] index :: the device/stream to push audio to.
	 * @param[in] sampleData :: pointer to audio samples buffer.
	 * @param[in] bufferSize :: size of buffer in bytes.
	 */
	template <IsPlainOldData T>
	void pushAudioData(unsigned index, T* sampleData, std::size_t bufferSize) const {
		pushRawAudio(index, sampleData, bufferSize, sizeof(T));
	}

	/**
	 * @brief Pushes buffer of audio samples to SDL device/stream.
	 * @param[in] index :: the device/stream to push audio to.
	 * @param[in] samplesBuffer :: audio samples buffer (C style).
	 */
	template <IsPlainOldData T, std::size_t N>
	void pushAudioData(unsigned index, T(&samplesBuffer)[N]) const {
		pushRawAudio(index, samplesBuffer, N, sizeof(T));
	}

	/**
	 * @brief Pushes buffer of audio samples to SDL device/stream.
	 * @param[in] index :: the device/stream to push audio to.
	 * @param[in] samplesBuffer :: audio samples buffer (C++ style).
	 */
	template <IsContiguousContainer T> requires (IsPlainOldData<ValueType<T>>)
	void pushAudioData(unsigned index, T& samplesBuffer) const {
		pushRawAudio(index, std::data(samplesBuffer), std::size(samplesBuffer), sizeof(ValueType<T>));
	}
};
