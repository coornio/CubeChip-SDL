/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstdint>
#include <string>

#include "Enums.hpp"

class HexInput;
struct FncSetInterface;

class ProgramControl final {
	FncSetInterface*& fncSet;

public:
	std::int32_t ipf{}, boost{};
	double framerate{};

	using enum Interrupt;
	Interrupt interrupt{ NONE };

	std::uint8_t timerDelay{};
	std::uint8_t timerSound{};

	explicit ProgramControl(FncSetInterface*&);
	std::string hexOpcode(std::uint32_t) const;

	void init(std::uint32_t&, std::uint32_t, std::int32_t);
	void setSpeed(std::int32_t);
	void setFncSet(FncSetInterface*);

	void requestHalt(std::uint32_t);
	void setInterrupt(bool, Interrupt);

	void handleTimersDec(bool&);
	void handleInterrupt(HexInput*, std::uint8_t&, bool&);
};
