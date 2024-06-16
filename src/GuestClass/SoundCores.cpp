/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>
#include <algorithm>

#include "../HostClass/BasicAudioSpec.hpp"
#include "../HostClass/BasicVideoSpec.hpp"

#include "ProgramControl.hpp"
#include "SoundCores.hpp"
#include "DisplayColors.hpp"
#include "Guest.hpp"

/*------------------------------------------------------------------*/
/*  class  SoundCores                                               */
/*------------------------------------------------------------------*/

SoundCores::SoundCores(VM_Guest* parent, BasicAudioSpec* bas)
	: vm { parent }
	, BAS{ bas }
{}

void SoundCores::renderAudio(
	BasicVideoSpec* BVS,
	DisplayColors*  Color,
	ProgramControl* Program
) {
	const auto samplesPerFrame{ std::ceil(BAS->outFrequency / Program->framerate) };
	std::vector<std::int16_t> audioBuffer(static_cast<std::uint32_t>(samplesPerFrame));

	if (beepFx0A) {
		C8.render(audioBuffer);
		BVS->AudioOutline(Color->buzz[1], Color->buzz[0]);
	} else if (MC.isOn()) {
		MC.render(audioBuffer);
		BVS->AudioOutline(0xFF'20'20'20, 0xFF'20'20'20);
	} else if (!Program->timerSound) {
		wavePhase = 0.0f;
		BVS->AudioOutline(Color->buzz[0], Color->buzz[0]);
	} else if (XO.isOn()) {
		XO.render(audioBuffer);
		BVS->AudioOutline(Color->buzz[0], Color->buzz[0]);
	} else {
		C8.render(audioBuffer);
		BVS->AudioOutline(Color->buzz[1], Color->buzz[0]);
	}

	BAS->pushAudioData(audioBuffer.data(), audioBuffer.size());
}

/*------------------------------------------------------------------*/
/*  class  SoundCores::Classic                                      */
/*------------------------------------------------------------------*/

SoundCores::Classic::Classic(SoundCores* parent, BasicAudioSpec* bas)
	: Sound{ parent }
	, BAS  { bas }
{}

void SoundCores::Classic::setTone(const std::size_t sp, const std::size_t pc) {
	// sets a unique tone for each sound call
	tone = (160.0f + 8.0f * ((pc >> 1) + sp + 1 & 0x3E)) / BAS->outFrequency;
}

void SoundCores::Classic::setTone(const std::size_t vx) {
	// sets the tone for each 8X sound call
	tone = (160.0f + ((0xFF - (vx ? vx : 0x7F)) >> 3 << 4)) / BAS->outFrequency;
}

void SoundCores::Classic::render(std::span<std::int16_t> buffer) {
	for (auto& sample : buffer) {
		sample = Sound->wavePhase > 0.5f ? BAS->amplitude : -BAS->amplitude;
		Sound->wavePhase = std::fmod(Sound->wavePhase + tone, 1.0f);
	}
}

/*------------------------------------------------------------------*/
/*  class  SoundCores::XOchip                                       */
/*------------------------------------------------------------------*/

SoundCores::XOchip::XOchip(SoundCores* parent, BasicAudioSpec* bas, VM_Guest* guest)
	: Sound{ parent }
	, BAS  { bas }
	, vm   { guest }
	, rate { 4000.0f / 128.0f / BAS->outFrequency }
	, tone { rate }
{}

bool SoundCores::XOchip::isOn() const {
	return enabled;
}

void SoundCores::XOchip::setPitch(const std::size_t pitch) {
	tone = rate * std::pow(2.0f, (pitch - 64.0f) / 48.0f);
	enabled = true;
}

void SoundCores::XOchip::loadPattern(std::size_t idx) {
	for (auto& byte : pattern) {
		byte = vm->mrw(idx++);

		if (!enabled && byte > 0x0 && byte < 0xFF)
			enabled = true;
	}
}

void SoundCores::XOchip::render(std::span<std::int16_t> buffer) {
	for (auto& sample : buffer) {
		const auto step{ static_cast<std::int32_t>(std::clamp(Sound->wavePhase * 128.0f, 0.0f, 127.0f)) };
		const auto mask{ 1 << (7 - (step & 7)) };
		sample = pattern[step >> 3] & mask ? BAS->amplitude : -BAS->amplitude;
		Sound->wavePhase = std::fmod(Sound->wavePhase + tone, 1.0f);
	}
}

/*------------------------------------------------------------------*/
/*  class  SoundCores::MegaChip                                     */
/*------------------------------------------------------------------*/

SoundCores::MegaChip::MegaChip(SoundCores* parent, BasicAudioSpec* bas, VM_Guest* guest)
	: Sound{ parent }
	, BAS  { bas }
	, vm   { guest }
{}

bool SoundCores::MegaChip::isOn() const {
	return enabled;
}

void SoundCores::MegaChip::reset(const bool state) {
	enabled = state;
	looping = false;

	length = 0;
	start  = 0;
	step   = 0.0;
	pos    = 0.0;
}

void SoundCores::MegaChip::enable(
	const std::size_t freq,
	const std::size_t len,
	const std::size_t offset,
	const bool loop
) {
	enabled = true;
	looping = loop;

	length = len;
	start  = offset;
	step   = freq * 1.0 / BAS->outFrequency;
	pos    = 0.0;
}

void SoundCores::MegaChip::render(std::span<std::int16_t> buffer) {
	for (auto& sample : buffer) {
		auto   _curidx{ vm->mrw(start + static_cast<std::uint32_t>(pos)) };
		double _offset{ pos + step };

		if (_offset >= length) {
			if (looping) {
				_offset -= length;
			} else {
				_offset = 0.0;
				_curidx = 128;
				length  = 0;
				enabled = false;
			}
		}
		pos = _offset;
		sample = static_cast<std::int16_t>(
			(_curidx - 128) * BAS->volume\
		);
	}
}
