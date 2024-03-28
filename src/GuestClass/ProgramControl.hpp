/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstdint>
#include <string>

class VM_Guest;
struct FncSetInterface;

enum class Interrupt {
    NONE,
    ONCE,
    STOP,
    WAIT,
    FX0A,
};

class ProgramControl final {
    VM_Guest& vm;
    FncSetInterface*& fncSet;

public:
    int32_t ipf{}, boost{};
    double framerate{};

    std::size_t limiter{};
    std::size_t screenMode{};
    uint32_t opcode{};
    uint32_t counter{};

    Interrupt interrupt{};

    struct TimerData {
        uint8_t delay{};
        uint8_t sound{};
    } Timer;

    explicit ProgramControl(VM_Guest&, FncSetInterface*&);
    std::string hexOpcode() const;

    void init(uint32_t, int32_t);
    void setSpeed(int32_t);
    void setFncSet(FncSetInterface*);

    void skipInstruction();
    void jumpInstruction(uint32_t);
    void stepInstruction(int32_t);

    void requestHalt();
    void setInterrupt(Interrupt);

    void handleTimersDec();
    void handleInterrupt();
};
