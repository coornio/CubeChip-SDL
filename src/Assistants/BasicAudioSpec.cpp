/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <algorithm>

#include "BasicAudioSpec.hpp"
#include "SettingWrapper.hpp"
#include "LifetimeWrapperSDL.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_messagebox.h>

/*==================================================================*/

BasicAudioSpec::BasicAudioSpec(const Settings& settings) noexcept {
	mStatus = SDL_InitSubSystem(SDL_INIT_AUDIO)
		? mStatus : STATUS::NO_AUDIO;

	setGlobalGain(settings.volume);
	isMuted(settings.muted);
}

BasicAudioSpec::~BasicAudioSpec() noexcept
	{ SDL_QuitSubSystem(SDL_INIT_AUDIO); }

/*==================================================================*/

SettingsMap BasicAudioSpec::Settings::map() noexcept {
	return {
		makeSetting("Audio.Volume", &volume),
		makeSetting("Audio.Muted",  &muted),
	};
}

auto BasicAudioSpec::exportSettings() const noexcept -> Settings {
	Settings out;

	out.volume = mGlobalGain.load(std::memory_order_relaxed);
	out.muted = mIsMuted.load(std::memory_order_relaxed);

	return out;
}

/*==================================================================*/

bool BasicAudioSpec::isMuted()           noexcept
	{ return mIsMuted.load(std::memory_order_relaxed); }

void BasicAudioSpec::isMuted(bool state) noexcept
	{ mIsMuted.store(state, std::memory_order_relaxed); }

void BasicAudioSpec::toggleMuted()       noexcept
	{ mIsMuted.store(isMuted(), std::memory_order_relaxed); }

/*==================================================================*/

float BasicAudioSpec::getGlobalGain() noexcept
	{ return mGlobalGain.load(std::memory_order_relaxed); }

void BasicAudioSpec::setGlobalGain(float gain) noexcept
	{ mGlobalGain.store(std::clamp(gain, 0.0f, 1.0f), std::memory_order_relaxed); }

void BasicAudioSpec::addGlobalGain(float gain) noexcept
	{ setGlobalGain(getGlobalGain() + gain); }

int BasicAudioSpec::getPlaybackDeviceCount() noexcept {
	auto deviceCount{ 0 };
	if (mStatus == STATUS::NORMAL) {
		SDL_Unique<SDL_AudioDeviceID> devices
			{ SDL_GetAudioPlaybackDevices(&deviceCount) };
	}
	return deviceCount;
}

int BasicAudioSpec::getRecordingDeviceCount() noexcept {
	auto deviceCount{ 0 };
	if (mStatus == STATUS::NORMAL) {
		SDL_Unique<SDL_AudioDeviceID> devices
			{ SDL_GetAudioRecordingDevices(&deviceCount) };
	}
	return deviceCount;
}
