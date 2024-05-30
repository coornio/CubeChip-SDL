/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <sstream>
#include <iomanip>

#include "../Assistants/BasicLogger.hpp"

#include "InstructionSets/Interface.hpp"
#include "HexInput.hpp"
#include "ProgramControl.hpp"
#include "SoundCores.hpp"
#include "Guest.hpp"

using namespace blogger;

ProgramControl::ProgramControl(VM_Guest* parent, FncSetInterface*& set)
	: vm    { parent }
	, fncSet{ set }
{}

std::string ProgramControl::hexOpcode() const {
	std::stringstream out;
	out << std::setfill('0') << std::setw(4)
		<< std::uppercase << std::hex
		<< opcode;
	return out.str();
}

void ProgramControl::init(
	const std::uint32_t _counter,
	const std::int32_t  _ipf
) {
	counter   = _counter;
	ipf       = _ipf;
	framerate = 60.0;
	interrupt = Interrupt::NONE;
}

void ProgramControl::setSpeed(const std::int32_t _ipf) {
	if (_ipf) ipf = _ipf;
	boost = (ipf < 50) ? (ipf >> 1) : 0;
}

void ProgramControl::setFncSet(FncSetInterface* _fncSet) {
	fncSet = _fncSet;
}

void ProgramControl::skipInstruction() {
	switch (vm->mrw(counter)) {
		case 0xF0: case 0xF1:
		case 0xF2: case 0xF3:
			if (!vm->mrw(counter + 1)) {
				counter += 2;
			}	
			break;

		case 0x01:
			counter += 2;
			break;
	}
	counter += 2;
}

void ProgramControl::jumpInstruction(const std::uint32_t next) {
	if ((counter - 2 & limiter) != next) [[likely]] {
		counter = next;
	} else {
		setInterrupt(Interrupt::STOP);
	}
}

void ProgramControl::stepInstruction(const std::int32_t step) {
	if (step) [[likely]] {
		counter = counter - 2 + step & limiter;
	} else {
		setInterrupt(Interrupt::STOP);
	}
}

void ProgramControl::setInterrupt(const Interrupt type) {
	interrupt = type;
	ipf *= -1;
}

void ProgramControl::requestHalt() {
	if (opcode & 0xF000) {
		blog.errLogOut("Unknown instruction detected: " + hexOpcode());
	} else {
		blog.errLogOut("ML routines are unsupported: " + hexOpcode());
	}
	setInterrupt(Interrupt::STOP);
}

void ProgramControl::handleTimersDec() {
	if (timerDelay) --timerDelay;
	if (timerSound) --timerSound;
	if (!timerSound) vm->Sound->beepFx0A = false;
}

void ProgramControl::handleInterrupt() {
	switch (interrupt) {

		case Interrupt::ONCE: // resumes emulation after a single frame pause
			ipf = std::abs(ipf);
			return;

		case Interrupt::STOP: // stops emulation when sound timer reaches 0
			if (!timerSound) ipf = 0;
			return;

		case Interrupt::WAIT: // pauses emulation while delay timer is not 0
			if (!timerDelay) {
				interrupt = Interrupt::NONE;
				ipf       = std::abs(ipf);
			}
			return;

		case Interrupt::FX0A: // resumes emulation when key press event for Fx0A
			if (vm->Input->keyPressed(vm->VX())) {
				interrupt  = Interrupt::NONE;
				ipf        = std::abs(ipf);
				timerSound = 2;
				vm->Sound->beepFx0A = true;
			}
			return;
	}
}
