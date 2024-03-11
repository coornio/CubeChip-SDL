/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "FrameLimiter.hpp"

FrameLimiter::FrameLimiter(
    const double framerate,
    const bool   firstpass,
    const bool   lostframe
) {
    setFreq(framerate, firstpass, lostframe);
}

void FrameLimiter::setFreq(
    const double framerate,
    const bool   firstpass,
    const bool   lostframe
) {
    timeFrequency = 1000.0 / std::clamp(framerate, 0.5, 1000.0);
    skipFirstPass = firstpass;
    skipLostFrame = lostframe;
}

bool FrameLimiter::operator()(const bool mode) {
    if (isValidFrame()) return true;

    if (mode == SLEEP && remains() >= 2.0)
        std::this_thread::sleep_for(millis(1));
    return false;
}

bool FrameLimiter::isValidFrame() {
    using namespace std::chrono;

    if (!initTimeCheck) [[unlikely]] {
        timePastFrame = steady_clock::now();
        initTimeCheck = true;
    }

    if (skipFirstPass) [[unlikely]] {
        skipFirstPass = false;
        ++validFrameCnt;
        return true;
    }

    timeVariation = {
        timeOvershoot + duration<double, std::milli>
        (steady_clock::now() - timePastFrame).count()
    };

    if (timeVariation < timeFrequency) [[likely]]
        return false;

    if (skipLostFrame) {
        lastFrameLost = timeVariation >= timeFrequency * 1.002f;
        timeOvershoot = std::fmod(timeVariation, timeFrequency);
    } else {
        timeOvershoot = timeVariation - timeFrequency;
    }

    timePastFrame = steady_clock::now();
    ++validFrameCnt;
    return true;
}
