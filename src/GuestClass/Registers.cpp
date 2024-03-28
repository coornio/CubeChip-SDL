/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <filesystem>
#include <fstream>

#include "Registers.hpp"
#include "Guest.hpp"
#include "../Assistants/BasicLogger.hpp"

using namespace blogger;

Registers::Registers(VM_Guest& parent)
	: vm(parent)
{}

void Registers::routineCall(const uint32_t addr) {
	stack[SP++ & 0xF] = vm.Program.counter;
	vm.Program.counter = addr;
}

void Registers::routineReturn() {
	vm.Program.counter = stack[--SP & 0xF];
}

void Registers::protectPages() {
	pageGuard = (3 - (V[0] - 1 & 0x3)) << 5;
}

bool Registers::readPermRegs(const std::size_t X) {
	static const std::filesystem::path sha1{
		vm.File.permRegs / vm.File.sha1
	};

	if (std::filesystem::exists(sha1)) {
		if (!std::filesystem::is_regular_file(sha1)) {
			blog.errLogOut("Log file is malformed: " + sha1.string());
			return false;
		}

		std::ifstream in(sha1, std::ios::binary);
		if (in.is_open()) {
			in.seekg(0, std::ios::end);
			const auto totalBytes{ static_cast<std::size_t>(in.tellg()) };
			in.seekg(0, std::ios::beg);

			in.read(reinterpret_cast<char*>(V.data()), std::min(totalBytes, X));
			in.close();

			if (totalBytes < X) {
				std::fill_n(V.begin() + totalBytes, X - totalBytes, uint8_t{ 0 });
			}
		} else {
			blog.errLogOut("Could not open SHA1 file to read: " + sha1.string());
			return false;
		}
	} else {
		std::fill_n(V.begin(), X, uint8_t{ 0 });
	}
	return true;
}

bool Registers::writePermRegs(const std::size_t X) {
	static const std::filesystem::path sha1{
		vm.File.permRegs / vm.File.sha1
	};

	if (std::filesystem::exists(sha1)) {
		if (!std::filesystem::is_regular_file(sha1)) {
			blog.errLogOut("Log file is malformed: " + sha1.string());
			return false;
		}

		std::array<char, 16> tempV{};
		std::ifstream in(sha1, std::ios::binary);

		if (in.is_open()) {
			in.seekg(0, std::ios::end);
			const auto totalBytes{ static_cast<std::size_t>(in.tellg()) };
			in.seekg(0, std::ios::beg);
			
			in.read(tempV.data(), std::min(totalBytes, X));
			in.close();
		} else {
			blog.errLogOut("Could not open SHA1 file to read: " + sha1.string());
			return false;
		}

		std::copy_n(V.begin(), X, tempV.begin());

		std::ofstream out(sha1, std::ios::binary);
		if (out.is_open()) {
			out.write(tempV.data(), tempV.size());
			out.close();
		} else {
			blog.errLogOut("Could not open SHA1 file to write: " + sha1.string());
			return false;
		}
	} else {
		std::ofstream out(sha1, std::ios::binary);
		if (out.is_open()) {
			out.write(reinterpret_cast<const char*>(V.data()), X);
			if (X < 16) {
				const std::vector<char> padding(16 - X, '\x00');
				out.write(padding.data(), padding.size());
			}
			out.close();
		} else {
			blog.errLogOut("Could not open SHA1 file to write: " + sha1.string());
			return false;
		}
	}
	return true;
}
