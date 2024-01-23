/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <chrono>
#include <ratio>
#include <thread>
#include <algorithm>

enum : bool { SPINLOCK, SLEEP };

class FrameLimiter final {
    using chrono = std::chrono::steady_clock::time_point;
    using millis = std::chrono::milliseconds;
    using uint64 = unsigned long long;

    bool   initTimeCheck{}; // updates timestamp on first check only
    bool   skipFirstPass{}; // unconditional valid frame on first check
    bool   skipLostFrame{}; // timeOvershoot will modulo with timeFrequency
    bool   lastFrameLost{}; // indicator of missed frame when using skipLostFrame

    double timeFrequency{}; // time (ms) per unit Hertz
    double timeOvershoot{}; // time remainder (ms) after last check
    double timeVariation{}; // time difference between frames
    chrono timePastFrame{}; // timestamp of the last frame check
    uint64 validFrameCnt{}; // counts total successful frame checks

    bool isValidFrame();

public:
    FrameLimiter(double = 60.0, bool = true, bool = false);
    void setFreq(double, bool, bool);
    bool operator()(bool = SPINLOCK);

    uint64 count()   const { return validFrameCnt; }
    double elapsed() const { return timeVariation; }
    double remains() const { return timeFrequency - timeVariation; }
    double percent() const { return timeVariation / timeFrequency; }
    bool   paced()   const { return timeOvershoot < timeFrequency && !lastFrameLost; }
};
