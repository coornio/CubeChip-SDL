/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../Assistants/BasicInput.hpp"
#include "../../Assistants/BasicLogger.hpp"
#include "../../Assistants/HomeDirManager.hpp"
#include "../../Assistants/BasicVideoSpec.hpp"
#include "../../Assistants/BasicAudioSpec.hpp"
#include "../../Assistants/Well512.hpp"

#include "Chip8_CoreInterface.hpp"

/*==================================================================*/

fsPath*   Chip8_CoreInterface::sPermaRegsPath{};
fsPath*   Chip8_CoreInterface::sSavestatePath{};

Chip8_CoreInterface::Chip8_CoreInterface() noexcept
	: ASB{ std::make_unique<AudioSpecBlock>(SDL_AUDIO_S16, 1, 48'000) }
{
	sSavestatePath = HDM->addSystemDir("savestate", "CHIP8");
	if (!sSavestatePath) { setCoreState(EmuState::FAILED); }
	sPermaRegsPath = HDM->addSystemDir("permaRegs", "CHIP8");
	if (!sPermaRegsPath) { setCoreState(EmuState::FAILED); }

	loadPresetBinds();
}

Chip8_CoreInterface::~Chip8_CoreInterface() noexcept {}

/*==================================================================*/

void Chip8_CoreInterface::loadPresetBinds() {
	static constexpr auto _{ SDL_SCANCODE_UNKNOWN };
	static constexpr SimpleKeyMapping defaultKeyMappings[]{
		{0x1, KEY(1), _}, {0x2, KEY(2), _}, {0x3, KEY(3), _}, {0xC, KEY(4), _},
		{0x4, KEY(Q), _}, {0x5, KEY(W), _}, {0x6, KEY(E), _}, {0xD, KEY(R), _},
		{0x7, KEY(A), _}, {0x8, KEY(S), _}, {0x9, KEY(D), _}, {0xE, KEY(F), _},
		{0xA, KEY(Z), _}, {0x0, KEY(X), _}, {0xB, KEY(C), _}, {0xF, KEY(V), _},
	};

	loadCustomBinds(defaultKeyMappings);
}

void Chip8_CoreInterface::loadCustomBinds(std::span<const SimpleKeyMapping> binds) {
	mCustomBinds.assign(binds.begin(), binds.end());
	mKeysPrev = mKeysCurr = mKeysLock = 0;
}

void Chip8_CoreInterface::updateKeyStates() {
	if (!mCustomBinds.size()) { return; }

	mKeysPrev = mKeysCurr;
	mKeysCurr = 0;

	for (const auto& mapping : mCustomBinds) {
		if (binput::kb.areAnyHeld(mapping.key, mapping.alt)) {
			mKeysCurr |= 1 << mapping.idx;
		}
	}

	mKeysLoop &= mKeysLock &= ~(mKeysPrev ^ mKeysCurr);
}

bool Chip8_CoreInterface::keyPressed(Uint8& returnKey, const Uint32 tickCount) noexcept {
	if (!mCustomBinds.size()) { return false; }

	if (tickCount >= mTickLast + mTickSpan) {
		mKeysPrev &= ~mKeysLoop;
	}

		const auto pressKeys{ mKeysCurr & ~mKeysPrev };
	if (pressKeys) {
		const auto pressDiff{ pressKeys & ~mKeysLoop };
		const auto validKeys{ pressDiff ? pressDiff : mKeysLoop };

		mKeysLock |= validKeys;
		mTickLast  = tickCount;
		mTickSpan  = validKeys != mKeysLoop ? 20 : 5;
		mKeysLoop  = validKeys & ~(validKeys - 1);
		returnKey  = std::countr_zero(mKeysLoop) & 0xFF;
	}
	return pressKeys;
}

bool Chip8_CoreInterface::keyHeld_P1(const Uint32 keyIndex) const noexcept {
	return mKeysCurr & ~mKeysLock & 0x01 << (keyIndex & 0xF);
}

bool Chip8_CoreInterface::keyHeld_P2(const Uint32 keyIndex) const noexcept {
	return mKeysCurr & ~mKeysLock & 0x10 << (keyIndex & 0xF);
}

/*==================================================================*/

void Chip8_CoreInterface::processFrame() {
	if (isSystemStopped()) { return; }
	else [[likely]] { ++mTotalFrames; }

	updateKeyStates();

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
