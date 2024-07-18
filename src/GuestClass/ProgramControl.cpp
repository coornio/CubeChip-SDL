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

using namespace blogger;

ProgramControl::ProgramControl(FncSetInterface*& set)
	: fncSet{ set }
{}

std::string ProgramControl::hexOpcode(const u32 opcode) const {
	std::stringstream out;
	out << std::setfill('0') << std::setw(4)
		<< std::uppercase    << std::hex
		<< opcode;
	return out.str();
}

void ProgramControl::init(
	      u32& pc_offset,
	const u32  _counter,
	const s32  _ipf
) {
	pc_offset = _counter;
	ipf       = _ipf;
	framerate = 60.0;
	interrupt = Interrupt::CLEAR;
}

void ProgramControl::setSpeed(const s32 _ipf) {
	if (_ipf) ipf = _ipf;
	boost = (ipf < 50) ? (ipf >> 1) : 0;
}

void ProgramControl::setFncSet(FncSetInterface* _fncSet) {
	fncSet = _fncSet;
}

void ProgramControl::setInterrupt(const Interrupt type) {
	interrupt = type;
	ipf = -std::abs(ipf);
}

void ProgramControl::triggerError(std::string_view msg) {
	blog.stdLogOut(msg.data());
	setInterrupt(Interrupt::ERROR);
}

void ProgramControl::triggerOpcodeError(const u32 opcode) {
	if (opcode & 0xF000) {
		blog.stdLogOut("Error :: Unknown instruction detected: " + hexOpcode(opcode));
	} else {
		blog.stdLogOut("Error :: ML routines are unsupported: " + hexOpcode(opcode));
	}
	setInterrupt(Interrupt::ERROR);
}

void ProgramControl::handleTimersDec(bool& beepFx0A) {
	if (timerDelay) --timerDelay;
	if (timerSound) --timerSound;
	if (!timerSound) beepFx0A = false;
}

void ProgramControl::handleInterrupt(bool& beepFx0A, HexInput* Input, u8& regVX) {
	switch (interrupt) {

		case Interrupt::FRAME: // resumes emulation after a single frame pause
			ipf = std::abs(ipf);
			return;

		case Interrupt::SOUND: // stops emulation when sound timer reaches 0
			if (!timerSound) {
				interrupt = Interrupt::FINAL;
				ipf       = 0;
			}
			return;

		case Interrupt::DELAY: // pauses emulation while delay timer is not 0
			if (!timerDelay) {
				interrupt = Interrupt::CLEAR;
				ipf       = std::abs(ipf);
			}
			return;

		case Interrupt::INPUT: // resumes emulation when key press event for Fx0A
			if (Input->keyPressed(regVX)) {
				interrupt  = Interrupt::CLEAR;
				ipf        = std::abs(ipf);
				timerSound = 2;
				beepFx0A   = true;
			}
			return;

		case Interrupt::FINAL:
		case Interrupt::ERROR:
			ipf = 0;
			return;
	}
}
