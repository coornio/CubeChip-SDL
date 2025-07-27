/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <algorithm>

#include "Concepts.hpp"
#include "ColorOps.hpp"
#include "AudioDevice.hpp"

/*==================================================================*/

constexpr inline float transientGain(unsigned iter, float step = 0.01f) noexcept
	{ return std::min(0.0f + step * (iter + 1), 1.0f); }

constexpr inline float transientFall(unsigned iter, float step = 0.01f) noexcept
	{ return std::max(1.0f - step * (iter + 1), 0.0f); }

/*==================================================================*/

struct TransienceGain {
	bool intro : 1;
	bool outro : 1;
	bool fallback : 1;

	constexpr TransienceGain() noexcept : fallback{ true } {}
	constexpr TransienceGain(bool intro, bool outro, bool fallback) noexcept
		: intro{ intro }, outro{ outro }, fallback{ fallback }
	{}

	constexpr auto calculate(unsigned sample_idx) const noexcept {
		return  intro ? ::transientGain(sample_idx) :
				outro ? ::transientFall(sample_idx) : fallback;
	}
};

/*==================================================================*/

/**
* @brief The AudioTimer represents how many frames a voice can be active.
* Internally, the state change when the AudioTimer is updated allows for trancient
* gain calculations, but they're not useful for data-driven voices.
*/
class AudioTimer {
	unsigned timer_old{};
	unsigned timer_new{};

public:
	constexpr unsigned get()        const noexcept { return timer_new; }
	constexpr void     set(unsigned time) noexcept { timer_old = std::exchange(timer_new, time); }
	constexpr void     dec()              noexcept { set(timer_new ? timer_new - 1 : 0); }

	// Check if the timer is currently rising (intro)
	constexpr bool intro() const noexcept { return timer_new && !timer_old; }

	// Check if the timer is currently falling (outro)
	constexpr bool outro() const noexcept { return !timer_new && timer_old; }

	constexpr operator unsigned() const noexcept { return timer_new; }

	constexpr operator TransienceGain() const noexcept
		{ return TransienceGain{ intro(), outro(), !!timer_new }; }
};

/*==================================================================*/

class Voice {
	using self = Voice;

protected:
	double mPhase{}; // [0..1) range.
	double mStep{};  // [0..1) range.
	float  mVolumeGain{}; // System-facing volume control, to be controlled by a system.
	float  mMasterGain{}; // Mastering volume control to manually balance against other voices.
	
public:
	// Pass along additional data to a voice processor, if needed.
	void* userdata{};

	Voice(float master_gain = 0.2f) noexcept
		: mVolumeGain{ 1.0f }
	{ setMasterGain(master_gain); }

	// Get the volume of the voice, in range of: [0..1]
	constexpr float getVolume()     const noexcept { return mVolumeGain; }
	// Set the volume of the voice, clamped to: [0..1]
	constexpr self& setVolume(float gain) noexcept { mVolumeGain = std::clamp(gain, 0.0f, 2.0f); return *this; }

	// Get the mastering volume of the voice, in range of: [0..1]
	constexpr float getMasterGain()     const noexcept { return mMasterGain; }
	// Set the mastering volume of the voice, clamped to: [0..1]
	constexpr self& setMasterGain(float gain) noexcept { mMasterGain = std::clamp(gain, 0.0f, 1.0f); return *this; }

	constexpr Phase getStep()     const noexcept { return mStep; }
	constexpr self& setStep(Phase step) noexcept { mStep = step; return *this; }

	constexpr Phase getPhase()      const noexcept { return mPhase; }
	constexpr self& setPhase(Phase phase) noexcept { mPhase = phase; return *this; }
	
	// Peek the next raw phase without wrapping it, default is 1 step ahead.
	constexpr double peekRawPhase(unsigned steps = 1u) const noexcept
		{ return mPhase + mStep * steps; }

	// Peek the next phase without modifying it, default is 1 step ahead.
	constexpr Phase peekPhase(unsigned steps = 1u) const noexcept
		{ return peekRawPhase(steps); }

	// Advance the phase by a number of steps, default is 1 step ahead.
	constexpr self& stepPhase(unsigned steps = 1u) noexcept
		{ mPhase = peekPhase(steps); return *this; }

	// Get the current level of the voice sample, optionally with transience gain calculation.
	constexpr float getLevel(unsigned sample_idx, TransienceGain transience = {}) const noexcept
		{ return transience.calculate(sample_idx) * getVolume() * getMasterGain(); }
};

/*==================================================================*/

using Stream = AudioDevice::Stream;

using SampleGenerator = void (*)(float*, unsigned, Voice*, Stream*) noexcept;

struct GeneratorBundle {
	SampleGenerator functor;
	Voice* voice;

	GeneratorBundle(SampleGenerator p, Voice* v) noexcept
		: functor{ p }, voice{ v }
	{}

	template <IsContiguousContainerOf<float> T>
	constexpr void run(T& buffer, Stream* stream) const noexcept {
		functor(std::data(buffer), unsigned(std::size(buffer)), voice, stream);
	}
};

using VoiceGenerators = std::initializer_list<GeneratorBundle>;
