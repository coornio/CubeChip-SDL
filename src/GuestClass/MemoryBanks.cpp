/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <utility>
#include <fstream>

#include "../Assistants/BasicLogger.hpp"
using namespace blogger;

#include "MemoryBanks.hpp"

void MemoryBanks::modifyViewport(
	const BrushType type,
	const s32       plane,
	const bool      xochip
) {
	if (!xochip) {
		displayBuffer[0].wipeAll();
		return;
	}

	for (auto P{ 0 }; P < 4; ++P) {
		if (!(plane & (1 << P))) { continue; }

		switch (type) {
			case BrushType::CLR:
				displayBuffer[P].wipeAll();
				break;
			case BrushType::XOR:
				for (auto& px : displayBuffer[P].span()) { px ^=  1; }
				break;
			case BrushType::SUB:
				for (auto& px : displayBuffer[P].span()) { px &= ~1; }
				break;
			case BrushType::ADD:
				for (auto& px : displayBuffer[P].span()) { px |=  1; }
				break;
		}
	}
}

void MemoryBanks::flushBuffers(const FlushType option) {
	switch (option) {
		case FlushType::DISCARD:
			for (auto& elem : megaPalette) { elem = {}; }
			break;

		case FlushType::DISPLAY:
			foregroundBuffer.copyLinear(backgroundBuffer);
			break;
	}

	backgroundBuffer.wipeAll();
	collisionPalette.wipeAll();
}

void MemoryBanks::loadPalette(const s32 count) {
	auto pos{ memIndex };
	for (auto idx{ 0 }; idx++ < count; pos += 4) {
		megaPalette[idx] = read(pos + 0) << 24
			             | read(pos + 1) << 16
			             | read(pos + 2) <<  8
			             | read(pos + 3);
	}
}

void MemoryBanks::clearPages(const s32 limit) {
	auto row{ pageGuard };
	while (row < limit) {
		displayBuffer[0][row++].wipeAll();
	}
}

//u8& MemoryBanks::Registers::VX() { return V[(opcode >> 8) & 0xF]; }

bool MemoryBanks::routineCall(const u32 addr) {
	stackBank[stackPtr++ & 0xF] = counter;
	counter = addr;
	return false;
	/*
	if (stackPtr < &*stackBank.end()) {
		*stackPtr++ = counter;
		counter = addr;
		return false;
	} else { return true; }
	*/
}

bool MemoryBanks::routineReturn() {
	counter = stackBank[--stackPtr & 0xF];
	return false;
	/*
	if (stackPtr >= &*stackBank.begin()) {
		counter = *(--stackPtr);
		return false;
	} else { return true; }
	*/
}

void MemoryBanks::protectPages() {
	pageGuard = (3 - (vRegister[0] - 1 & 0x3)) << 5;
}

bool MemoryBanks::readPermRegs(HomeDirManager* HDM, const usz X) {
	static const std::filesystem::path sha1{
		HDM->permRegs / HDM->sha1
	};

	if (std::filesystem::exists(sha1)) {
		if (!std::filesystem::is_regular_file(sha1)) {
			blog.stdLogOut("SHA1 file is malformed: " + sha1.string());
			return false;
		}

		std::ifstream in(sha1, std::ios::binary);
		if (in.is_open()) {
			in.seekg(0, std::ios::end);
			const auto totalBytes{ static_cast<usz>(in.tellg()) };
			in.seekg(0, std::ios::beg);

			in.read(reinterpret_cast<char*>(vRegister), std::min(totalBytes, X));
			in.close();

			if (totalBytes < X) {
				std::fill_n(vRegister + totalBytes, X - totalBytes, u8());
			}
		} else {
			blog.stdLogOut("Could not open SHA1 file to read: " + sha1.string());
			return false;
		}
	} else {
		std::fill_n(vRegister, X, u8());
	}
	return true;
}

bool MemoryBanks::writePermRegs(HomeDirManager* HDM, const usz X) {
	static const std::filesystem::path sha1{
		HDM->permRegs / HDM->sha1
	};

	if (std::filesystem::exists(sha1)) {
		if (!std::filesystem::is_regular_file(sha1)) {
			blog.stdLogOut("SHA1 file is malformed: " + sha1.string());
			return false;
		}

		char tempV[16]{};
		std::ifstream in(sha1, std::ios::binary);

		if (in.is_open()) {
			in.seekg(0, std::ios::end);
			const auto totalBytes{ in.tellg() };
			in.seekg(0, std::ios::beg);

			in.read(tempV, std::min<std::streamsize>(totalBytes, X));
			in.close();
		} else {
			blog.stdLogOut("Could not open SHA1 file to read: " + sha1.string());
			return false;
		}

		std::copy_n(vRegister, X, tempV);

		std::ofstream out(sha1, std::ios::binary);
		if (out.is_open()) {
			out.write(tempV, 16);
			out.close();
		} else {
			blog.stdLogOut("Could not open SHA1 file to write: " + sha1.string());
			return false;
		}
	} else {
		std::ofstream out(sha1, std::ios::binary);
		if (out.is_open()) {
			out.write(reinterpret_cast<const char*>(vRegister), X);
			if (X < 16) {
				const char padding[16]{};
				out.write(padding, 16 - X);
			}
			out.close();
		} else {
			blog.stdLogOut("Could not open SHA1 file to write: " + sha1.string());
			return false;
		}
	}
	return true;
}

void MemoryBanks::skipInstruction() {
	switch (read(counter + 0)) {
		case 0x01:
			counter += 4;
			break;

		case 0xF0: case 0xF1:
		case 0xF2: case 0xF3:
			counter += (read(counter + 1)) ? 2 : 4;
			break;

		default:
			counter += 2;
	}
}

bool MemoryBanks::jumpInstruction(const u32 next) {
	if (counter - 2 != next) [[likely]] {
		counter = next;
		return false;
	}
	return true;
}

bool MemoryBanks::stepInstruction(const s32 step) {
	if (step) [[likely]] {
		counter += step - 2;
		return false;
	}
	return true;
}