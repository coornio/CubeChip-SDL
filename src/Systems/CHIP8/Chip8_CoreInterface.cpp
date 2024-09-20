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
std::array<u8, 240> Chip8_CoreInterface::sFontsData{ Chip8_CoreInterface::cFontsData };
std::array<u32, 16> Chip8_CoreInterface::sBitColors{ Chip8_CoreInterface::cBitColors };

Chip8_CoreInterface::Chip8_CoreInterface() noexcept
	: ASB{ std::make_unique<AudioSpecBlock>(SDL_AUDIO_S8, 1, 48'000) }
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

bool Chip8_CoreInterface::keyPressed(u8* returnKey, const u32 tickCount) noexcept {
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
		*returnKey = std::countr_zero(mKeysLoop) & 0xFF;
	}
	return pressKeys;
}

bool Chip8_CoreInterface::keyHeld_P1(const u32 keyIndex) const noexcept {
	return mKeysCurr & ~mKeysLock & 0x01 << (keyIndex & 0xF);
}

bool Chip8_CoreInterface::keyHeld_P2(const u32 keyIndex) const noexcept {
	return mKeysCurr & ~mKeysLock & 0x10 << (keyIndex & 0xF);
}

/*==================================================================*/

void Chip8_CoreInterface::handlePreFrameInterrupt() noexcept {
	switch (mInterrupt)
	{
		case Interrupt::FRAME:
			mInterrupt = Interrupt::CLEAR;
			mActiveCPF = std::abs(mActiveCPF);
			return;

		case Interrupt::SOUND:
			if (!mSoundTimer) {
				mInterrupt = Interrupt::FINAL;
				mActiveCPF = 0;
			}
			return;

		case Interrupt::DELAY:
			if (!mSoundTimer) {
				mInterrupt = Interrupt::CLEAR;
				mActiveCPF = std::abs(mActiveCPF);
			}
			return;
	}
}

void Chip8_CoreInterface::handleEndFrameInterrupt() noexcept {
	switch (mInterrupt)
	{
		case Interrupt::INPUT:
			if (keyPressed(mInputReg, mTotalFrames)) {
				mInterrupt  = Interrupt::CLEAR;
				mActiveCPF  = std::abs(mActiveCPF);
				mBuzzerTone = calcBuzzerTone();
				mSoundTimer = 2;
				isBuzzerEnabled(true);
			}
			return;

		case Interrupt::ERROR:
		case Interrupt::FINAL:
			setCoreState(EmuState::HALTED);
			mActiveCPF = 0;
			return;
	}
}

void Chip8_CoreInterface::handleTimerTick() noexcept {
	if (mDelayTimer) { --mDelayTimer; }
	if (mSoundTimer) { --mSoundTimer; }
}

void Chip8_CoreInterface::nextInstruction() noexcept {
	mCurrentPC += 2;
}

void Chip8_CoreInterface::skipInstruction() noexcept {
	mCurrentPC += 2;
}

void Chip8_CoreInterface::performProgJump(const u32 next) noexcept {
	const auto NNN{ next & 0xFFF };
	if (mCurrentPC - 2u != NNN) [[likely]] {
		mCurrentPC = NNN & 0xFFF;
	} else {
		triggerInterrupt(Interrupt::SOUND);
	}
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

/*==================================================================*/

std::string Chip8_CoreInterface::formatOpcode(const u32 OP) const {
	char buffer[5];
	std::format_to(buffer, "{:04X}{}", OP, '\0');
	return buffer;
}

void Chip8_CoreInterface::instructionError(const u32 HI, const u32 LO) {
	blog.newEntry(BLOG::INFO, "Unknown instruction: " + formatOpcode(HI << 8 | LO));
	triggerInterrupt(Interrupt::ERROR);
}

void Chip8_CoreInterface::triggerInterrupt(const Interrupt type) noexcept {
	mInterrupt = type;
	mActiveCPF = -std::abs(mActiveCPF);
}

void Chip8_CoreInterface::triggerCritError(const std::string& msg) noexcept {
	blog.newEntry(BLOG::INFO, msg);
	triggerInterrupt(Interrupt::ERROR);
}

/*==================================================================*/

bool Chip8_CoreInterface::setPermaRegs(const s32 X) noexcept {
	const auto path{ *sPermaRegsPath / HDM->getFileSHA1() };

	if (std::filesystem::exists(path)) {
		if (!std::filesystem::is_regular_file(path)) {
			blog.newEntry(BLOG::ERROR, "SHA1 file is malformed: " + path.string());
			return true;
		}

		char tempV[16]{};
		std::ifstream in(path, std::ios::binary);

		if (in.is_open()) {
			in.seekg(0, std::ios::end);
			const auto totalBytes{ in.tellg() };
			in.seekg(0, std::ios::beg);

			in.read(tempV, std::min<std::streamsize>(totalBytes, X));
			in.close();
		} else {
			blog.newEntry(BLOG::ERROR, "Could not open SHA1 file to read: " + path.string());
			return true;
		}

		std::copy_n(mRegisterV, X, tempV);

		std::ofstream out(path, std::ios::binary);
		if (out.is_open()) {
			out.write(tempV, 16);
			out.close();
		} else {
			blog.newEntry(BLOG::ERROR, "Could not open SHA1 file to write: " + path.string());
			return true;
		}
	} else {
		std::ofstream out(path, std::ios::binary);
		if (out.is_open()) {
			out.write(reinterpret_cast<const char*>(mRegisterV), X);
			if (X < 16) {
				const char padding[16]{};
				out.write(padding, 16 - X);
			}
			out.close();
		} else {
			blog.newEntry(BLOG::ERROR, "Could not open SHA1 file to write: " + path.string());
			return true;
		}
	}
	return false;
}

bool Chip8_CoreInterface::getPermaRegs(const s32 X) noexcept {
	const auto path{ *sPermaRegsPath / HDM->getFileSHA1() };

	if (std::filesystem::exists(path)) {
		if (!std::filesystem::is_regular_file(path)) {
			blog.newEntry(BLOG::ERROR, "SHA1 file is malformed: " + path.string());
			return true;
		}

		std::ifstream in(path, std::ios::binary);
		if (in.is_open()) {
			in.seekg(0, std::ios::end);
			const auto totalBytes{ static_cast<s32>(in.tellg()) };
			in.seekg(0, std::ios::beg);

			in.read(reinterpret_cast<char*>(mRegisterV), std::min(totalBytes, X));
			in.close();

			if (totalBytes < X) {
				std::fill_n(mRegisterV + totalBytes, X - totalBytes, u8());
			}
		} else {
			blog.newEntry(BLOG::ERROR, "Could not open SHA1 file to read: " + path.string());
			return true;
		}
	} else {
		std::fill_n(
			std::execution::unseq,
			mRegisterV, X, u8{}
		);
	}
	return false;
}

/*==================================================================*/

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
		sFontsData.begin(), size, dest + offset
	);
}

void Chip8_CoreInterface::copyColorsToCore(
	u32* dest, const u32 size
) noexcept {
	std::copy_n(
		std::execution::unseq,
		sBitColors.begin(), size, dest
	);
}

/*==================================================================*/

f32  Chip8_CoreInterface::calcBuzzerTone() const noexcept {
	return (160.0f + 8.0f * (
		(mCurrentPC >> 1) + mStackTop + 1 & 0x3E
	)) / ASB->getFrequency();
}
