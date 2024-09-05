/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cmath>
#include <chrono>
#include <ratio>
#include <thread>
#include <cstdint>
#include <optional>
#include <algorithm>

class FrameLimiter final {
	using chrono = std::chrono::steady_clock::time_point;
	using millis = std::chrono::milliseconds;
	using uint64 = std::uint64_t;

	bool   initTimeCheck{}; // forces timestamp update on first check only
	bool   skipFirstPass{}; // forces valid frame return on first check only
	bool   skipLostFrame{}; // forces frameskip if timeOvershoot > timeFrequency
	bool   lastFrameLost{}; // missed frame indicator when frameskip is enabled

	float  timeFrequency{}; // holds time (ms) per unit Hertz
	float  timeOvershoot{}; // holds time remainder (ms) from last successful check
	float  timeVariation{}; // holds time difference between last check and now
	chrono timePastFrame{}; // holds timestamp of the last frame's check
	uint64 validFrameCnt{}; // counter of successful frame checks performed

	bool isValidFrame() noexcept;
	auto getElapsedTime() const noexcept {
		return std::chrono::steady_clock::now() - timePastFrame;
	}

public:
	FrameLimiter(
		float  framerate = 60.0f, // 0.5 ... 1000 range
		bool   firstpass = true,  // skipFirstPass flag
		bool   lostframe = true   // skipLostFrame flag
	) noexcept {
		setLimiter(framerate, firstpass, lostframe);
	}

	FrameLimiter(const FrameLimiter& other) noexcept
		: skipFirstPass{ other.skipFirstPass }
		, skipLostFrame{ other.skipLostFrame }
		, timeFrequency{ other.timeFrequency }
	{}

	void setLimiter(
		float               framerate,
		std::optional<bool> firstpass = std::nullopt,
		std::optional<bool> lostframe = std::nullopt
	) noexcept;

	enum : bool { SPINLOCK, SLEEP };
	bool checkTime(bool mode = SLEEP);

	auto getElapsedMillisSince() const noexcept {
		using tmillis = std::chrono::milliseconds;
		return duration_cast<tmillis>(getElapsedTime()).count();
	}

	auto getElapsedMicrosSince() const noexcept {
		using tmicros = std::chrono::microseconds;
		return duration_cast<tmicros>(getElapsedTime()).count();
	}

	auto getValidFrameCounter() const noexcept { return validFrameCnt; }
	auto getElapsedMillisLast() const noexcept { return timeVariation; }
	auto getFramespan()         const noexcept { return timeFrequency; }
	auto getRemainder()         const noexcept { return timeVariation - timeFrequency; }
	auto getPercentage()        const noexcept { return timeVariation / timeFrequency; }
	bool isKeepingPace()        const noexcept { return timeOvershoot < timeFrequency && !lastFrameLost; }
};
