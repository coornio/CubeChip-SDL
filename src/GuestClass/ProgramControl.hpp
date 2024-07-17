/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstdint>
#include <string>

#include "Enums.hpp"
#include "../Types.hpp"

class HexInput;
struct FncSetInterface;

class ProgramControl final {
	FncSetInterface*& fncSet;

public:
	s32 ipf{}, boost{};
	double framerate{};

	using enum Interrupt;
	Interrupt interrupt{ NONE };

	u8 timerDelay{};
	u8 timerSound{};

	explicit ProgramControl(FncSetInterface*&);
	std::string hexOpcode(u32) const;

	void init(u32&, u32, s32);
	void setSpeed(s32);
	void setFncSet(FncSetInterface*);

	void requestHalt(u32);
	void setInterrupt(bool, Interrupt);

	void handleTimersDec(bool&);
	void handleInterrupt(bool&, HexInput*, u8&);
};
