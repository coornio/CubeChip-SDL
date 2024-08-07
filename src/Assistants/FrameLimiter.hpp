/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

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

	bool isValidFrame();

public:
	FrameLimiter(
		float  framerate = 60.0f, // 0.5 ... 1000 range
		bool   firstpass = true,  // skipFirstPass flag
		bool   lostframe = true   // skipLostFrame flag
	) {
		setLimiter(framerate, firstpass, lostframe);
	}

	FrameLimiter(const FrameLimiter& other)
		: skipFirstPass{ other.skipFirstPass }
		, skipLostFrame{ other.skipLostFrame }
		, timeFrequency{ other.timeFrequency }
	{}

	void setLimiter(
		float               framerate,
		std::optional<bool> firstpass = std::nullopt,
		std::optional<bool> lostframe = std::nullopt
	);

	enum : bool { SPINLOCK, SLEEP };
	bool   check(bool mode = SPINLOCK);

	uint64 count()   const { return validFrameCnt; }
	float  elapsed() const { return timeVariation; }
	float  remains() const { return timeFrequency - timeVariation; }
	float  percent() const { return timeVariation / timeFrequency; }
	bool   paced()   const { return timeOvershoot < timeFrequency && !lastFrameLost; }
};
