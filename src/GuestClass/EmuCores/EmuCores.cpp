/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <fstream>

#include "EmuCores.hpp"
#include "../Enums.hpp"

#include "../../HostClass/HomeDirManager.hpp"
#include "../../HostClass/BasicVideoSpec.hpp"
#include "../../HostClass/BasicAudioSpec.hpp"

#include "../../Assistants/BasicLogger.hpp"
using namespace blogger;

EmuCores::~EmuCores() noexcept = default;
EmuCores::EmuCores(
	HomeDirManager& ref_HDM,
	BasicVideoSpec& ref_BVS,
	BasicAudioSpec& ref_BAS
) noexcept
	: HDM{ ref_HDM }
	, BVS{ ref_BVS }
	, BAS{ ref_BAS }
{}

void EmuCores::setInterrupt(const Interrupt type) {
	mInterruptType  = type;
	mCyclesPerFrame = -std::abs(mCyclesPerFrame);
}

void EmuCores::operationError(std::string_view msg) {
	blog.stdLogOut(msg.data());
	setInterrupt(Interrupt::ERROR);
}

std::string EmuCores::formatOpcode(const u32 HI, const u32 LO) const {
	std::stringstream out;
	out << std::setfill('0') << std::setw(2)
		<< std::uppercase    << std::hex
		<< HI << LO;
	return out.str();
}

void EmuCores::instructionError(const u32 HI, const u32 LO) {
	blog.stdLogOut("Error :: Unknown instruction: " + formatOpcode(HI, LO));
	setInterrupt(Interrupt::ERROR);
}

void EmuCores::instructionErrorML(const u32 HI, const u32 LO) {
	blog.stdLogOut("Error :: ML routines unsupported: " + formatOpcode(HI, LO));
	setInterrupt(Interrupt::ERROR);
}

bool EmuCores::copyGameToMemory(u8* dest, const u32 offset) {
	std::basic_ifstream<char> ifs(HDM.path, std::ios::binary);
	ifs.read(reinterpret_cast<char*>(dest + offset), HDM.size);
	if (ifs.fail()) {
		blog.stdLogOut("Failed to copy rom data to memory, aborting.");
		return false;
	} else {
		return true;
	}
}

void EmuCores::copyFontToMemory(u8* dest, const u32 offset, const u32 size) {
	std::copy_n(
		std::execution::unseq,
		cFontData, size, dest + offset
	);
}

bool VM_Guest::initGameCore(
	HomeDirManager& HDM,
	BasicVideoSpec& BVS,
	BasicAudioSpec& BAS
) {
	mCoreBase = std::move(GameFileChecker::initializeCore(HDM, BVS, BAS));
	return mCoreBase ? true : false;
}
