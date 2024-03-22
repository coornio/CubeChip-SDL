/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Guest.hpp"
#include "../HostClass/Host.hpp"

/*------------------------------------------------------------------*/
/*  class  VM_Guest::ProgramControl                                 */
/*------------------------------------------------------------------*/

VM_Guest::ProgramControl::ProgramControl(VM_Guest& parent, FncSetInterface*& set)
    : vm(parent)
    , fncSet(set)
{}

std::string VM_Guest::ProgramControl::hexOpcode() const {
    std::stringstream out;
    out << std::setfill('0') << std::setw(4)
        << std::uppercase    << std::hex
        << opcode;
    return out.str();
}

void VM_Guest::ProgramControl::init(const u32 _counter, const s32 _ipf) {
    counter   = _counter;
    ipf       = _ipf;
    framerate = 60.0;
    interrupt = Interrupt::NONE;
}

void VM_Guest::ProgramControl::setSpeed(const s32 _ipf) {
    if (_ipf) ipf = _ipf;
    boost = (ipf < 50) ? (ipf >> 1) : 0;
}

void VM_Guest::ProgramControl::setFncSet(FncSetInterface* _fncSet) {
    fncSet = _fncSet;
}

void VM_Guest::ProgramControl::skipInstruction() {
    switch (vm.mrw(counter)) {
        case 0xF0:
        case 0xF1:
        case 0xF2:
        case 0xF3:
            if (!vm.mrw(counter + 1))
                counter += 2;
            break;
        case 0x01:
            counter += 2;
            break;
    }
    counter += 2;
}

void VM_Guest::ProgramControl::jumpInstruction(const u32 next) {
    if ((counter - 2 & limiter) == next) [[unlikely]]
        setInterrupt(Interrupt::STOP);
    else counter = next;
}

void VM_Guest::ProgramControl::stepInstruction(const s32 step) {
    if (!step) [[unlikely]]
        setInterrupt(Interrupt::STOP);
    else counter = counter - 2 + step & limiter;
}

void VM_Guest::ProgramControl::setInterrupt(const Interrupt type) {
    interrupt = type;
    ipf *= -1;
}

void VM_Guest::ProgramControl::requestHalt() {
    setInterrupt(Interrupt::STOP);
    switch (opcode & 0xF000) {
        case 0x0:
            blog.errLogOut("ML routines are unsupported: " + hexOpcode());
            return;
        default:
            blog.errLogOut("Unknown instruction detected: " + hexOpcode());
            return;
    }
}

void VM_Guest::ProgramControl::handleTimersDec() {
    if (Timer.delay) --Timer.delay;
    if (Timer.sound) --Timer.sound;
    if (!Timer.sound) vm.Audio.beepFx0A = false;
}

void VM_Guest::ProgramControl::handleInterrupt() {
    switch (interrupt) {

        case Interrupt::ONCE: // resumes emulation after a single frame pause
            ipf = std::abs(ipf);
            return;

        case Interrupt::STOP: // stops emulation when sound timer reaches 0
            if (!Timer.sound) ipf = 0;
            return;

        case Interrupt::WAIT: // pauses emulation while delay timer is not 0
            if (!Timer.delay) {
                interrupt = Interrupt::NONE;
                ipf       = std::abs(ipf);
            }
            return;

        case Interrupt::FX0A: // resumes emulation when key press event for Fx0A
            if (vm.Input.keyPressed(vm.VX())) {
                interrupt   = Interrupt::NONE;
                ipf         = std::abs(ipf);
                Timer.sound = 2;
                vm.Audio.beepFx0A = true;
            }
            return;
    }
}
