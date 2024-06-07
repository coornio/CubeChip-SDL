/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <array>
#include <vector>

class VM_Guest;

class BasicAudioSpec;
class BasicVideoSpec;

class DisplayColors;
class ProgramControl;

class SoundCores final {
	VM_Guest*       vm;
	BasicAudioSpec* BAS;

public:
	float wavePhase{};
	bool  beepFx0A{};

	explicit SoundCores(VM_Guest*, BasicAudioSpec*);
	void renderAudio(BasicVideoSpec*, DisplayColors*, ProgramControl*);

	/*------------------------------------------------------------------*/

	class Classic final {
		SoundCores*     Sound;
		BasicAudioSpec* BAS;

		float tone{};

	public:
		explicit Classic(SoundCores*, BasicAudioSpec*);

		void setTone(std::size_t, std::size_t);
		void setTone(std::size_t);
		void render(std::span<std::int16_t>);
	} C8{ this, BAS };

	/*------------------------------------------------------------------*/

	class XOchip final {
		SoundCores*     Sound;
		BasicAudioSpec* BAS;
		VM_Guest*       vm;

		const float rate;
		std::array<std::uint8_t, 16> pattern{};
		float tone{};
		bool  enabled{};

	public:
		explicit XOchip(SoundCores*, BasicAudioSpec*, VM_Guest*);
		bool isOn() const;

		void setPitch(std::size_t);
		void loadPattern(std::size_t);
		void render(std::span<std::int16_t>);
	} XO{ this, BAS, vm };

	/*------------------------------------------------------------------*/

	class MegaChip final {
		SoundCores*     Sound;
		BasicAudioSpec* BAS;
		VM_Guest*       vm;

		std::size_t length{};
		std::size_t start{};
		double step{};
		double pos{};
		bool enabled{};
		bool looping{};

	public:
		explicit MegaChip(SoundCores*, BasicAudioSpec*, VM_Guest*);
		bool isOn() const;

		void reset(bool);
		void enable(std::size_t, std::size_t, std::size_t, bool);
		void render(std::span<std::int16_t>);
	} MC{ this, BAS, vm };
};
