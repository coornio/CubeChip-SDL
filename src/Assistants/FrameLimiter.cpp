/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "FrameLimiter.hpp"

void FrameLimiter::setLimiter(
	const float               framerate,
	const std::optional<bool> firstpass,
	const std::optional<bool> lostframe
) noexcept {
	timeFrequency = 1000.0f / std::clamp(framerate, 0.5f, 1000.0f);
	if (firstpass) { skipFirstPass = *firstpass; }
	if (lostframe) { skipLostFrame = *lostframe; }
}

bool FrameLimiter::checkTime(const bool mode) {
	if (isValidFrame()) { return true; }

	if (mode == SLEEP && getRemainder() >= 2.0f) {
		std::this_thread::sleep_for(millis(1));
	}
	return false;
}

bool FrameLimiter::isValidFrame() noexcept {
	using namespace std::chrono;
	const auto timeAtCurrent{ steady_clock::now() };

	if (!initTimeCheck) [[unlikely]] {
		timePastFrame = timeAtCurrent;
		initTimeCheck = true;
	}

	if (skipFirstPass) [[unlikely]] {
		skipFirstPass = false;
		++validFrameCnt;
		return true;
	}

	timeVariation = {
		timeOvershoot + duration<float, std::milli>
		(timeAtCurrent - timePastFrame).count()
	};

	if (timeVariation < timeFrequency) [[likely]] {
		return false;
	}

	if (skipLostFrame) {
		lastFrameLost = timeVariation >= timeFrequency * 1.003f;
		timeOvershoot = std::fmod(timeVariation, timeFrequency);
	} else {
		timeOvershoot = timeVariation - timeFrequency;
	}

	timePastFrame = timeAtCurrent;
	++validFrameCnt;
	return true;
}
