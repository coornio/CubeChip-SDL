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
#include "MemoryBanks.hpp"

using namespace blogger;

ProgramControl::ProgramControl(FncSetInterface*& set)
	: fncSet{ set }
{}

std::string ProgramControl::hexOpcode(const std::uint32_t opcode) const {
	std::stringstream out;
	out << std::setfill('0') << std::setw(4)
		<< std::uppercase    << std::hex
		<< opcode;
	return out.str();
}

void ProgramControl::init(
	std::uint32_t& pc_offset,
	const std::uint32_t _counter,
	const std::int32_t  _ipf
) {
	pc_offset = _counter;
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

void ProgramControl::setInterrupt(const bool cond, const Interrupt type) {
	if (!cond) { return; }
	interrupt = type;
	ipf *= -1;
}

void ProgramControl::requestHalt(const std::uint32_t opcode) {
	if (opcode & 0xF000) {
		blog.stdLogOut("Unknown instruction detected: " + hexOpcode(opcode));
	} else {
		blog.stdLogOut("ML routines are unsupported: " + hexOpcode(opcode));
	}
	setInterrupt(true, Interrupt::STOP);
}

void ProgramControl::handleTimersDec(bool& beepFx0A) {
	if (timerDelay) --timerDelay;
	if (timerSound) --timerSound;
	if (!timerSound) beepFx0A = false;
}

void ProgramControl::handleInterrupt(HexInput* Input, std::uint8_t& regVX, bool& beepFx0A) {
	switch (interrupt) {

		case Interrupt::ONCE: // resumes emulation after a single frame pause
			ipf = std::abs(ipf);
			return;

		case Interrupt::STOP: // stops emulation when sound timer reaches 0
			if (!timerSound) { ipf = 0; }
			return;

		case Interrupt::WAIT: // pauses emulation while delay timer is not 0
			if (!timerDelay) {
				interrupt = Interrupt::NONE;
				ipf       = std::abs(ipf);
			}
			return;

		case Interrupt::FX0A: // resumes emulation when key press event for Fx0A
			if (Input->keyPressed(regVX)) {
				interrupt  = Interrupt::NONE;
				ipf        = std::abs(ipf);
				timerSound = 2;
				beepFx0A   = true;
			}
			return;
	}
}
