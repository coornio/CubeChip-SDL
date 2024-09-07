/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

//#include <fstream>
#include <format>

#include "../../Assistants/BasicLogger.hpp"
#include "../../Assistants/HomeDirManager.hpp"
#include "../../Assistants/BasicVideoSpec.hpp"
#include "../../Assistants/BasicAudioSpec.hpp"
#include "../../Assistants/Well512.hpp"

#include "CoreInterface.hpp"

#include "HexInput.hpp"

/*==================================================================*/

std::filesystem::path* Chip8_CoreInterface::sPermaRegsPath{};
std::filesystem::path* Chip8_CoreInterface::sSavestatePath{};

HexInput* Chip8_CoreInterface::Input{};

Chip8_CoreInterface::Chip8_CoreInterface() noexcept {
	static HexInput sInput;
	Input = &sInput;

	Input->loadPresetBinds();

	sPermaRegsPath = HDM->addSystemDir("permaRegs", "CHIP8");
	sSavestatePath = HDM->addSystemDir("savestate", "CHIP8");
	if (!sPermaRegsPath || !sSavestatePath) { setCoreState(EmuState::FAILED); }
}

void Chip8_CoreInterface::processFrame() {
	if (isSystemStopped()) { return; }
	else [[likely]] { ++mTotalFrames; }

	Input->updateKeyStates();

	handleTimerTick();
	handlePreFrameInterrupt();
	instructionLoop();
	handleEndFrameInterrupt();

	renderAudioData();
	renderVideoData();
}

void Chip8_CoreInterface::triggerInterrupt(const Interrupt type) noexcept {
	mInterruptType  = type;
	mCyclesPerFrame = -std::abs(mCyclesPerFrame);
}

void Chip8_CoreInterface::triggerCritError(const std::string& msg) noexcept {
	blog.newEntry(BLOG::INFO, msg);
	triggerInterrupt(Interrupt::ERROR);
}

std::string Chip8_CoreInterface::formatOpcode(const u32 OP) const {
	char buffer[5];
	std::format_to(buffer, "{:04X}{}", OP, '\0');
	return buffer;
}

void Chip8_CoreInterface::instructionError(const u32 HI, const u32 LO) {
	blog.newEntry(BLOG::INFO, "Unknown instruction: " + formatOpcode(HI << 8 | LO));
	triggerInterrupt(Interrupt::ERROR);
}

void Chip8_CoreInterface::copyGameToMemory(
	u8* dest, const u32 offset
) noexcept {
	std::copy_n(
		std::execution::unseq,
		HDM->getFileData(),
		HDM->getFileSize(),
		dest + offset
	);
}

void Chip8_CoreInterface::copyFontToMemory(
	u8* dest, const u32 offset, const u32 size
) noexcept {
	std::copy_n(
		std::execution::unseq,
		cFontData, size, dest + offset
	);
}
