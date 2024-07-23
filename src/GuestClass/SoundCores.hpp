/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <span>
#include <array>
#include <vector>

#include "../Types.hpp"

class BasicAudioSpec;

class SoundCores final {
	f32 mWavePhase{};

public:
	bool  beepFx0A{};

	explicit SoundCores(BasicAudioSpec&);
	void renderAudio(BasicAudioSpec&, u32*, const u32*, f32, bool);

	/*------------------------------------------------------------------*/

	class Classic final {
		const s32 mFreq;
		f32 mTone{};

	public:
		explicit Classic(s32);

		void setTone(u32, u32);
		void setTone(u32);
		void render(std::span<s16>, s16, f32&) const;
	} C8;

	/*------------------------------------------------------------------*/

	class XOchip final {
		bool mEnabled{};
		const f32 mRate;
		u8   mPattern[16]{};
		f32  mTone{};

	public:
		explicit XOchip(s32);
		bool isEnabled() const { return mEnabled; }

		void setPitch(usz);
		bool loadPattern(const std::span<const u8>, const u32);
		void render(std::span<s16>, s16, f32&) const;
	} XO;

	/*------------------------------------------------------------------*/

	class MegaChip final {
		const f32  mFreq;
		      s32  mLen{};
		const u8*  pMem{};

		f64 mInc{};
		f64 mPos{};

	public:
		explicit MegaChip(s32);
		bool isEnabled() const { return mLen != 0; }

		void reset();
		bool initTrack(const std::span<const u8>, const u32, const bool);
		void render(std::span<s16>, s16);
	} MC;
};
