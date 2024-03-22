/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Guest.hpp"
#include "../HostClass/Host.hpp"

/*------------------------------------------------------------------*/
/*  class  VM_Guest::Registers                                      */
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
	static const std::filesystem::path sha1{
		vm.Host.File.permRegs / vm.Host.File.sha1
	};

	if (std::filesystem::exists(sha1)) {
		if (!std::filesystem::is_regular_file(sha1)) {
			blog.errLogOut("Log file is malformed: " + sha1.string());
			return false;
		}

		std::ifstream in(sha1, std::ios::binary);
		if (in.is_open()) {
			in.seekg(0, std::ios::end);
			const auto totalBytes{ as<usz>(in.tellg()) };
			in.seekg(0, std::ios::beg);

			in.read(to<char*>(V.data()), std::min(totalBytes, X));
			in.close();

			if (totalBytes < X) {
				std::fill_n(V.begin() + totalBytes, X - totalBytes, u8{ 0 });
			}
		} else {
			blog.errLogOut("Could not open SHA1 file to read: " + sha1.string());
			return false;
		}
	} else {
		std::fill_n(V.begin(), X, u8{ 0 });
	}
	return true;
}

bool VM_Guest::Registers::writePermRegs(const usz X) {
	static const std::filesystem::path sha1{
		vm.Host.File.permRegs / vm.Host.File.sha1
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
			const auto totalBytes{ as<usz>(in.tellg()) };
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
			out.write(to<const char*>(V.data()), X);
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
