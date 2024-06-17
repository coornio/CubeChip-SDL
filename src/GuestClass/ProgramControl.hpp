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
	VM_Guest* vm;
	FncSetInterface*& fncSet;

public:
	std::int32_t ipf{}, boost{};
	double framerate{};

	std::size_t   limiter{};
	std::uint32_t opcode{};
	std::uint32_t counter{};

	Interrupt interrupt{};

	bool screenLores{};
	bool screenHires{};

	std::uint8_t timerDelay{};
	std::uint8_t timerSound{};

	explicit ProgramControl(VM_Guest*, FncSetInterface*&);
	std::string hexOpcode() const;

	void init(std::uint32_t, std::int32_t);
	void setSpeed(std::int32_t);
	void setFncSet(FncSetInterface*);

	void skipInstruction();
	void jumpInstruction(std::uint32_t);
	void stepInstruction(std::int32_t);

	void requestHalt();
	void setInterrupt(Interrupt);

	void handleTimersDec();
	void handleInterrupt();
};
