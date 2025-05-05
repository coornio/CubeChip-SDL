/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <atomic>

#include "Typedefs.hpp"
#include "SettingWrapper.hpp"

/*==================================================================*/
	#pragma region BasicAudioSpec Singleton Class

class BasicAudioSpec final {
	static inline std::atomic<float> mGlobalGain{};
	static inline std::atomic<bool>  mIsMuted{};
	static inline bool mSuccessful{ true };

public:
	struct Settings {
		float volume{ 0.75f };
		bool  muted{ false };

		SettingsMap map() noexcept {
			return {
				makeSetting("Audio.Volume", &volume),
				makeSetting("Audio.Muted",  &muted),
			};
		}
	};

	[[nodiscard]]
	auto exportSettings() const noexcept {
		Settings out;

		out.volume = mGlobalGain.load(mo::relaxed);
		out.muted  = mIsMuted.load(mo::relaxed);

		return out;
	}

private:
	BasicAudioSpec(const Settings& settings) noexcept;
	~BasicAudioSpec() noexcept;
	BasicAudioSpec(const BasicAudioSpec&) = delete;
	BasicAudioSpec& operator=(const BasicAudioSpec&) = delete;

public:
	static auto* initialize(const Settings& settings) noexcept {
		static BasicAudioSpec self(settings);
		return mSuccessful ? &self : nullptr;
	}

	static bool isSuccessful() noexcept { return mSuccessful; }

	static bool isMuted()           noexcept { return mIsMuted.load(mo::relaxed); }
	static void isMuted(bool state) noexcept { mIsMuted.store(state, mo::relaxed); }
	static void toggleMuted()       noexcept { mIsMuted.store(isMuted(), mo::relaxed); }

	static auto getGlobalGain()     noexcept { return mGlobalGain.load(mo::relaxed); }
	static auto getGlobalGainByte() noexcept { return static_cast<signed>(getGlobalGain() * 255.0f); }

	static void setGlobalGain(float  gain) noexcept;
	static void addGlobalGain(float  gain) noexcept;
	static void addGlobalGain(signed gain) noexcept;
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
