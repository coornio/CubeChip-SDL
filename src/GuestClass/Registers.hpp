/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class VM_Guest;
class HomeDirManager;

class Registers final {
	VM_Guest*       vm;
	HomeDirManager* HDM;

public:
	std::array<std::uint32_t, 16> stack{};
	std::array<std::uint8_t,  16> V{};
	std::uint32_t I{}, SP{}, pageGuard{};

	explicit Registers(VM_Guest*, HomeDirManager*);
	void routineCall(std::uint32_t);
	void routineReturn();
	void protectPages();
	bool readPermRegs(std::size_t);
	bool writePermRegs(std::size_t);
};
