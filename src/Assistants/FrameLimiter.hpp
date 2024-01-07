#pragma once

#include <chrono>
#include <ratio>
#include <thread>
#include <type_traits>
#include <algorithm>

enum : bool { SPINLOCK, SLEEP };

class FrameLimiter {
    using chrono = std::chrono::steady_clock::time_point;
    using millis = std::chrono::milliseconds;
    using uint64 = unsigned long long;

    bool   initTimeCheck{}; // updates timestamp on first check only
    bool   timeSkipFirst{}; // unconditional valid frame on first check
    bool   skipOvershoot{}; // timeOvershoot will never overtake timeFrequency

    double timeFrequency{}; // time (ms) per unit Hertz
    double timeOvershoot{}; // time remainder (ms) after last check
    double timeVariation{}; // time difference between frames
    chrono timePastFrame{}; // timestamp of the last frame check
    uint64 validFrameCnt{}; // counts total successful frame checks

    bool isValidFrame();

public:
    FrameLimiter(double = 60.0, bool = true);
    void setFreq(double, bool);
    bool operator()(bool = SPINLOCK);

    uint64 count()   const { return validFrameCnt; }
    double elapsed() const { return timeVariation; }
    double remains() const { return timeFrequency - timeVariation; }
    double percent() const { return timeVariation / timeFrequency; }
    bool   paced()   const { return timeOvershoot < timeFrequency; }
};
