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

class FrameLimiter final {
    using chrono = std::chrono::steady_clock::time_point;
    using millis = std::chrono::milliseconds;
    using uint64 = unsigned long long;

    bool   initTimeCheck{}; // forces timestamp update on first check only
    bool   skipFirstPass{}; // forces valid frame return on first check only
    bool   skipLostFrame{}; // forces frameskip if timeOvershoot > timeFrequency
    bool   lastFrameLost{}; // missed frame indicator when frameskip is enabled

    double timeFrequency{}; // holds time (ms) per unit Hertz
    double timeOvershoot{}; // holds time remainder (ms) from last successful check
    double timeVariation{}; // holds time difference between last check and now
    chrono timePastFrame{}; // holds timestamp of the last frame's check
    uint64 validFrameCnt{}; // counter of successful frame checks performed

    bool isValidFrame();

public:
    explicit FrameLimiter(
        const double framerate = 60.0, // 0.5 ... 1000 range
        const bool   firstpass = true, // skipFirstPass flag
        const bool   lostframe = false // skipLostFrame flag
    );
    void setFreq(
        const double framerate, // 0.5 ... 1000 range
        const bool   firstpass, // skipFirstPass flag
        const bool   lostframe  // skipLostFrame flag
    );

    enum : bool { SPINLOCK, SLEEP };
    bool operator()(const bool mode = SPINLOCK);

    uint64 count()   const { return validFrameCnt; }
    double elapsed() const { return timeVariation; }
    double remains() const { return timeFrequency - timeVariation; }
    double percent() const { return timeVariation / timeFrequency; }
    bool   paced()   const { return timeOvershoot < timeFrequency && !lastFrameLost; }
};
