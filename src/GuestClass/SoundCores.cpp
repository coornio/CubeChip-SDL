/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>
#include <algorithm>

#include "../HostClass/BasicAudioSpec.hpp"

#include "SoundCores.hpp"

/*------------------------------------------------------------------*/
/*  class  SoundCores                                               */
/*------------------------------------------------------------------*/

SoundCores::SoundCores(BasicAudioSpec* BAS)
	: C8{ BAS->getFrequency() }
	, XO{ BAS->getFrequency() }
	, MC{ BAS->getFrequency() }
{}

void SoundCores::renderAudio(
	BasicAudioSpec*  BAS,
	      u32* const colorDst,
	const u32* const colorSrc,
	const double     framerate,
	const bool       buzzer
) {
	const auto samplesPerFrame{ std::ceil(BAS->getFrequency() / framerate) };
	std::vector<s16> audioBuffer(static_cast<u32>(samplesPerFrame));

	if (beepFx0A) {
		C8.render(audioBuffer, BAS->getAmplitude(), &wavePhase);
		colorDst[2] = colorSrc[1]; colorDst[1] = colorSrc[0];
	} else if (MC.isEnabled()) {
		MC.render(audioBuffer, BAS->getVolume());
		colorDst[2] = colorDst[1] = 0xFF202020;
	} else if (!buzzer) {
		wavePhase = 0.0f;
		colorDst[2] = colorDst[1] = colorSrc[0];
	} else if (XO.isEnabled()) {
		XO.render(audioBuffer, BAS->getAmplitude(), &wavePhase);
		colorDst[2] = colorDst[1] = colorSrc[0];
	} else {
		C8.render(audioBuffer, BAS->getAmplitude(), &wavePhase);
		colorDst[2] = colorSrc[1]; colorDst[1] = colorSrc[0];
	}

	BAS->pushAudioData(audioBuffer.data(), audioBuffer.size());
}

/*------------------------------------------------------------------*/
/*  class  SoundCores::Classic                                      */
/*------------------------------------------------------------------*/

SoundCores::Classic::Classic(s32 frequency)
	: freq{ frequency }
{}

void SoundCores::Classic::setTone(const u32 sp, const u32 pc) {
	// sets a unique tone for each sound call
	tone = (160.0f + 8.0f * ((pc >> 1) + sp + 1 & 0x3E)) / freq;
}

void SoundCores::Classic::setTone(const u32 vx) {
	// sets the tone for each 8X sound call
	tone = (160.0f + ((0xFF - (vx ? vx : 0x7F)) >> 3 << 4)) / freq;
}

void SoundCores::Classic::render(
	std::span<s16> buffer,
	const     s16  amplitude,
	float*  const  wavePhase
) const {
	for (auto& sample : buffer) {
		sample = *wavePhase > 0.5f ? amplitude : -amplitude;
		*wavePhase = std::fmod(*wavePhase + tone, 1.0f);
	}
}

/*------------------------------------------------------------------*/
/*  class  SoundCores::XOchip                                       */
/*------------------------------------------------------------------*/

SoundCores::XOchip::XOchip(s32 frequency)
	: rate { 4000.0f / 128.0f / frequency }
	, tone { rate }
{}

void SoundCores::XOchip::setPitch(const usz pitch) {
	tone = rate * std::pow(2.0f, (pitch - 64.0f) / 48.0f);
	enabled = true;
}

bool SoundCores::XOchip::loadPattern(
	const std::span<const u8> mem,
	const u32 index
) {
	if (index + 16 >= mem.size()) {
		return true;
	} else {
		enabled = true;
		for (auto idx{ 0 }; idx < 16; ++idx) {
			pattern[idx] = mem[index + idx];
		}
		return false;
	}
}

void SoundCores::XOchip::render(
	std::span<s16> buffer,
	const     s16  amplitude,
	float* const   wavePhase
) const {
	for (auto& sample : buffer) {
		const auto step{ static_cast<s32>(std::clamp(*wavePhase * 128.0f, 0.0f, 127.0f)) };
		const auto mask{ 1 << (7 ^ step & 7) };
		sample = pattern[step >> 3] & mask ? amplitude : -amplitude;
		*wavePhase = std::fmod(*wavePhase + tone, 1.0f);
	}
}

/*------------------------------------------------------------------*/
/*  class  SoundCores::MegaChip                                     */
/*------------------------------------------------------------------*/

SoundCores::MegaChip::MegaChip(s32 frequency)
	: mFreq{ static_cast<float>(frequency) }
{}

void SoundCores::MegaChip::reset() {
	pMem = nullptr;
	mInc = mPos = mLen = 0;
}

bool SoundCores::MegaChip::initTrack(
	const std::span<const u8> mem,
	const u32  idx,
	const bool rep
) {
	if (idx + 6 >= mem.size()) { // minimum requirement
		reset();
		return true;
	}

	pMem = mem.data() + idx;
	mLen = pMem[2] << 16 | pMem[3] << 8 | pMem[4];

	if (mLen && idx + 6 + mLen < mem.size()) { // now we check the full range
		mInc  = (pMem[0] << 8 | pMem[1]) * 1.0 / mFreq;
		mLen  =  rep ? -mLen : mLen;
		mPos  =  0.0;
		pMem +=  6;
		return false;
	} else {
		reset();
		return true;
	}
}

void SoundCores::MegaChip::render(std::span<s16> buffer, s16 volume) {
	for (auto& sample : buffer) {
		sample = volume * (pMem[static_cast<u32>(mPos)] - 128);

		if ((mPos += mInc) >= std::abs(mLen)) {
			if (mLen < 0) {
				mPos += mLen;
			} else {
				reset();
				return;
			}
		}
	}
}
