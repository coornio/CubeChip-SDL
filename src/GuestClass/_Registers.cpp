/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Guest.hpp"
#include "../HostClass/Host.hpp"

/*------------------------------------------------------------------*/
/*  struct  VM_Guest::Registers                                     */
/*------------------------------------------------------------------*/

VM_Guest::Registers::Registers(VM_Guest& parent)
	: vm(parent)
{}

void VM_Guest::Registers::routineCall(const u32 addr) {
	stack[SP++ & 0xF] = vm.Program.counter;
	vm.Program.counter = addr;
}

void VM_Guest::Registers::routineReturn() {
	vm.Program.counter = stack[--SP & 0xF];
}

void VM_Guest::Registers::protectPages() {
	pageGuard = (3 - (V[0] - 1 & 0x3)) << 5;
}

bool VM_Guest::Registers::readPermRegs(const usz X) {
	namespace fs = std::filesystem;

	const std::string& sha1path{ vm.Host.File.sha1 };
	const fs::path fspath{ "../../regdata/"s + sha1path };

	if (fs::exists(fspath)) {
		if (!fs::is_regular_file(fspath)) {
			vm.Host.addMessage("SHA1 path doesn't lead to file? : "s + fspath.string(), false);
			return false;
		}

		std::ifstream in(fspath, std::ios::binary);

		if (!in.is_open()) {
			vm.Host.addMessage("Could not open SHA1 file to read!"s, false, vm.Program.opcode);
			return false;
		}

		// calculate bytes in file
		in.seekg(0, std::ios::end);
		const auto totalBytes{ as<usz>(in.tellg()) };
		in.seekg(0, std::ios::beg);
		// read eligible bytes to V regs
		in.read(to<char*>(V.data()), std::min(totalBytes, X));
		in.close();

		// if eligible bytes were less, zero out the rest
		if (totalBytes < X) {
			std::fill_n(V.begin() + totalBytes, X - totalBytes, u8{ 0 });
		}

	}
	else {
		std::fill_n(V.begin(), X, u8{ 0 });
	}
	return true;
}

bool VM_Guest::Registers::writePermRegs(const usz X) {
	namespace fs = std::filesystem;

	const std::string& sha1path{ vm.Host.File.sha1 };
	const fs::path fspath{ "../../regdata/"s + sha1path };

	if (fs::exists(fspath)) {
		if (!fs::is_regular_file(fspath)) {
			vm.Host.addMessage("SHA1 path doesn't lead to file? : "s + fspath.string(), false);
			return false;
		}

		std::array<char, 16> tempRegs{};
		std::ifstream in(fspath, std::ios::binary);

		if (!in.is_open()) {
			vm.Host.addMessage("Could not open SHA1 file to read!"s, false, vm.Program.opcode);
			return false;
		}

		// calculate bytes in file
		in.seekg(0, std::ios::end);
		const auto totalBytes{ as<usz>(in.tellg()) };
		in.seekg(0, std::ios::beg);
		// read eligible bytes to temp array
		in.read(tempRegs.data(), std::min(totalBytes, X));
		in.close();

		// copy and overwrite eligible bytes from V to temp
		std::copy_n(V.begin(), X, tempRegs.begin());

		std::ofstream out(fspath, std::ios::binary);
		if (!out.is_open()) {
			vm.Host.addMessage("Could not open SHA1 file to write!"s, false, vm.Program.opcode);
			return false;
		}
		// write all temp bytes back to the file
		out.write(tempRegs.data(), tempRegs.size());
		out.close();

	}
	else {
		std::ofstream out(fspath, std::ios::binary);
		if (!out.is_open()) return false;

		out.write(to<const char*>(V.data()), X);

		if (X < 16) {
			const std::vector<char> padding(16 - X, '\x00');
			out.write(padding.data(), padding.size());
		}
		out.close();
	}
	return true;
}
