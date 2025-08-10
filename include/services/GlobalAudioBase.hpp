/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <atomic>

#include "SettingWrapper.hpp"

/*==================================================================*/

class GlobalAudioBase final {
	static inline std::atomic<float> mGlobalGain{};
	static inline std::atomic<bool>  mIsMuted{};

public:
	enum STATUS : bool { NORMAL, NO_AUDIO };

private:
	static inline STATUS mStatus{ STATUS::NORMAL };

public:
	struct Settings {
		float volume{ 0.75f };
		bool  muted{ false };

		SettingsMap map() noexcept;
	};

	[[nodiscard]]
	auto exportSettings() const noexcept -> Settings;

private:
	GlobalAudioBase(const Settings& settings) noexcept;
	~GlobalAudioBase() noexcept;
	GlobalAudioBase(const GlobalAudioBase&) = delete;
	GlobalAudioBase& operator=(const GlobalAudioBase&) = delete;

public:
	static auto* initialize(const Settings& settings) noexcept {
		static GlobalAudioBase self(settings);
		return &self;
	}

	static bool getStatus() noexcept { return mStatus; }

	static bool isMuted()           noexcept;
	static void isMuted(bool state) noexcept;
	static void toggleMuted()       noexcept;

	static float getGlobalGain()           noexcept;
	static void  setGlobalGain(float gain) noexcept;
	static void  addGlobalGain(float gain) noexcept;

	static int getPlaybackDeviceCount() noexcept;
	static int getRecordingDeviceCount() noexcept;
};
