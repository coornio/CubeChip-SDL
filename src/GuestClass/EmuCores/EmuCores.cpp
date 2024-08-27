/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <fstream>
#include <sstream>
#include <iomanip>

#include "EmuCores.hpp"

#include "../../HostClass/HomeDirManager.hpp"
#include "../../HostClass/BasicVideoSpec.hpp"
#include "../../HostClass/BasicAudioSpec.hpp"

#include "../../Assistants/BasicLogger.hpp"
using namespace blogger;

Chip8_CoreInterface::Chip8_CoreInterface(
	HomeDirManager& ref_HDM,
	BasicVideoSpec& ref_BVS,
	BasicAudioSpec& ref_BAS
) noexcept
	: HDM{ ref_HDM }
	, BVS{ ref_BVS }
	, BAS{ ref_BAS }
{}

void Chip8_CoreInterface::setInterrupt(const Interrupt type) {
	mInterruptType  = type;
	mCyclesPerFrame = -std::abs(mCyclesPerFrame);
}

void Chip8_CoreInterface::operationError(std::string_view msg) {
	blog.newEntry(BLOG::INFO, msg.data());
	setInterrupt(Interrupt::ERROR);
}

std::string Chip8_CoreInterface::formatOpcode(const u32 HI, const u32 LO) const {
	std::stringstream out;
	out << std::setfill('0') << std::setw(2)
		<< std::uppercase    << std::hex << HI
		<< std::setfill('0') << std::setw(2)
		<< std::uppercase    << std::hex << LO;
	return out.str();
}

void Chip8_CoreInterface::instructionError(const u32 HI, const u32 LO) {
	blog.newEntry(BLOG::INFO, "Unknown instruction: " + formatOpcode(HI, LO));
	setInterrupt(Interrupt::ERROR);
}

void Chip8_CoreInterface::instructionErrorML(const u32 HI, const u32 LO) {
	blog.newEntry(BLOG::INFO, "ML routines unsupported: " + formatOpcode(HI, LO));
	setInterrupt(Interrupt::ERROR);
}

void Chip8_CoreInterface::copyGameToMemory(u8* dest, const u32 offset) {
	std::copy_n(
		std::execution::unseq,
		HDM.getFileData(), HDM.getFileSize(), dest + offset
	);
}

void Chip8_CoreInterface::copyFontToMemory(u8* dest, const u32 offset, const u32 size) {
	std::copy_n(
		std::execution::unseq,
		cFontData, size, dest + offset
	);
}
