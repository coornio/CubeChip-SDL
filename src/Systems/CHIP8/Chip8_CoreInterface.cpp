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
#include "../../Assistants/SimpleFileIO.hpp"
#include "../../Assistants/Well512.hpp"

#include "Chip8_CoreInterface.hpp"

/*==================================================================*/

Chip8_CoreInterface::Chip8_CoreInterface() noexcept
	: ASB{ std::make_unique<AudioSpecBlock>(SDL_AUDIO_S8, 1, 48'000, STREAM::COUNT) }
{
	if ((sSavestatePath = HDM->addSystemDir("savestate", "CHIP8"))) {
		*sSavestatePath /= HDM->getFileSHA1();
		if (!checkFileValidity(*sSavestatePath)) { sSavestatePath = nullptr; }
	}
	if ((sPermaRegsPath = HDM->addSystemDir("permaRegs", "CHIP8"))) {
		*sPermaRegsPath /= HDM->getFileSHA1();
		if (!checkFileValidity(*sPermaRegsPath)) { sPermaRegsPath = nullptr; }
	}

	ASB->resumeStreams();
	loadPresetBinds();
}

Chip8_CoreInterface::~Chip8_CoreInterface() noexcept {}

/*==================================================================*/

void Chip8_CoreInterface::updateKeyStates() {
	if (!std::size(mCustomBinds)) { return; }

	mKeysPrev = mKeysCurr;
	mKeysCurr = 0;

	for (const auto& mapping : mCustomBinds) {
		if (binput::kb.areAnyHeld(mapping.key, mapping.alt)) {
			mKeysCurr |= 1 << mapping.idx;
		}
	}

	mKeysLoop &= mKeysLock &= ~(mKeysPrev ^ mKeysCurr);
}

void Chip8_CoreInterface::loadPresetBinds() {
	static constexpr auto _{ SDL_SCANCODE_UNKNOWN };
	static constexpr SimpleKeyMapping defaultKeyMappings[]{
		{0x1, KEY(1), _}, {0x2, KEY(2), _}, {0x3, KEY(3), _}, {0xC, KEY(4), _},
		{0x4, KEY(Q), _}, {0x5, KEY(W), _}, {0x6, KEY(E), _}, {0xD, KEY(R), _},
		{0x7, KEY(A), _}, {0x8, KEY(S), _}, {0x9, KEY(D), _}, {0xE, KEY(F), _},
		{0xA, KEY(Z), _}, {0x0, KEY(X), _}, {0xB, KEY(C), _}, {0xF, KEY(V), _},
	};

	loadCustomBinds(std::span(defaultKeyMappings));
}

bool Chip8_CoreInterface::keyPressed(u8* returnKey, const u32 tickCount) noexcept {
	if (!std::size(mCustomBinds)) { return false; }

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
			if (mAudioTimer[0]) { return; }
			if (mAudioTimer[1]) { return; }
			if (mAudioTimer[2]) { return; }
			if (mAudioTimer[3]) { return; }
			mInterrupt = Interrupt::FINAL;
			mActiveCPF = 0;
			return;

		case Interrupt::DELAY:
			if (!mDelayTimer) {
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
				mInterrupt = Interrupt::CLEAR;
				mActiveCPF = std::abs(mActiveCPF);
				startAudioAtChannel(STREAM::BUZZER, 2);
			}
			return;

		case Interrupt::ERROR:
			addCoreState(EmuState::FATAL);
			mActiveCPF = 0;
			return;

		case Interrupt::FINAL:
			setCoreState(EmuState::HALTED);
			mActiveCPF = 0;
			return;
	}
}

void Chip8_CoreInterface::handleTimerTick() noexcept {
	if (mDelayTimer)    { --mDelayTimer; }
	if (mAudioTimer[0]) { --mAudioTimer[0]; }
	if (mAudioTimer[1]) { --mAudioTimer[1]; }
	if (mAudioTimer[2]) { --mAudioTimer[2]; }
	if (mAudioTimer[3]) { --mAudioTimer[3]; }
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

void Chip8_CoreInterface::startAudio(const s32 duration, const s32 tone) noexcept {
	if constexpr(STREAM::COUNT == 0) { return; }
	
	static auto index{ 0 };
	startAudioAtChannel(index, duration, tone);
	++index %= STREAM::COUNT - 1;
}

void Chip8_CoreInterface::startAudioAtChannel(const u32 index, const s32 duration, const s32 tone) noexcept {
	if (index >= STREAM::COUNT) { return; }

	mAudioTimer[index] = static_cast<u8>(duration);
	mPhaseStep[index] = (sTonalOffset + (tone ? tone
		: 8 * ((mCurrentPC >> 1) + mStackTop + 1 & 0x3E)
	)) / ASB->getFrequency();
}

void Chip8_CoreInterface::pushSquareTone(const u32 index, const f32 framerate) noexcept {
	std::vector<s8> samplesBuffer \
		(static_cast<usz>(ASB->getSampleRate(framerate)));

	if (mAudioTimer[index]) {
		for (auto& audioSample : samplesBuffer) {
			audioSample        = static_cast<s8>(mAudioPhase[index] > 0.5f ? 16 : -16);
			mAudioPhase[index] = std::fmod(mAudioPhase[index] + mPhaseStep[index], 1.0f);
		}
	} else { mAudioPhase[index] = 0.0f; }

	ASB->pushAudioData(index, samplesBuffer);
}

void Chip8_CoreInterface::instructionError(const u32 HI, const u32 LO) {
	blog.newEntry(BLOG::INFO, "Unknown instruction: 0x{:04X}", HI << 8 | LO);
	triggerInterrupt(Interrupt::ERROR);
}

void Chip8_CoreInterface::triggerInterrupt(const Interrupt type) noexcept {
	mInterrupt = type;
	mActiveCPF = -std::abs(mActiveCPF);
}

/*==================================================================*/

bool Chip8_CoreInterface::checkFileValidity(const Path& filePath) noexcept {
	if (filePath.empty()) { return false; }

	const auto fileExists{ fs::exists(filePath) };
	if (!fileExists) {
		blog.newEntry(BLOG::ERROR, "\"{}\" [{}]",
			filePath.string(), fileExists.error().message()
		);
		return false;
	}

	if (fileExists.value()) {
		const auto fileNormal{ fs::is_regular_file(filePath) };
		if (!fileNormal) {
			blog.newEntry(BLOG::ERROR, "\"{}\" [{}]",
				filePath.string(), fileNormal.error().message()
			);
			return false;
		}

		if (fileNormal.value()) { return true; }
		else {
			const auto fileRemove{ fs::remove(filePath) };
			if (!fileRemove) {
				blog.newEntry(BLOG::ERROR, "\"{}\" [{}]",
					filePath.string(), fileRemove.error().message()
				);
				return false;
			}

			if (fileRemove.value()) { return true; }
			else {
				blog.newEntry(BLOG::WARN, "{}: \"{}\"",
					"Cannot remove irregular file", filePath.string()
				);
				return false;
			}
		}
	} else {
		const char blankRegs[sPermRegsV.size()]{};
		const auto fileWritten{ ::writeFileData(filePath, blankRegs) };
		if (fileWritten) { return true; }
		else {
			blog.newEntry(BLOG::WARN, "{}: \"{}\"",
				"Cannot write new file", filePath.string()
			);
			return false;
		}
	}
}

void Chip8_CoreInterface::setFilePermaRegs(const u32 X) noexcept {
	auto fileData{ ::writeFileData(*sPermaRegsPath, mRegisterV, X) };
	if (!fileData) {
		blog.newEntry(BLOG::ERROR, "File IO error: \"{}\" [{}]",
			sPermaRegsPath->string(), fileData.error().message()
		);
	}
}

void Chip8_CoreInterface::getFilePermaRegs(const u32 X) noexcept {
	auto fileData{ ::readFileData(*sPermaRegsPath, X) };
	if (!fileData) {
		blog.newEntry(BLOG::ERROR, "File IO error: \"{}\" [{}]",
			sPermaRegsPath->string(), fileData.error().message()
		);
	} else {
		std::copy_n(fileData.value().begin(), X, sPermRegsV.begin());
	}
}

void Chip8_CoreInterface::setPermaRegs(const u32 X) noexcept {
	if (sPermaRegsPath) {
		if (checkFileValidity(*sPermaRegsPath)) { setFilePermaRegs(X); }
		else { sPermaRegsPath = nullptr; }
	}
	std::copy_n(mRegisterV.begin(), X, sPermRegsV.begin());
}

void Chip8_CoreInterface::getPermaRegs(const u32 X) noexcept {
	if (sPermaRegsPath) {
		if (checkFileValidity(*sPermaRegsPath)) { getFilePermaRegs(X); }
		else { sPermaRegsPath = nullptr; }
	}
	std::copy_n(sPermRegsV.begin(), X, mRegisterV.begin());
}

/*==================================================================*/

void Chip8_CoreInterface::copyGameToMemory(void* dest) noexcept {
	std::memcpy(dest, HDM->getFileData(), HDM->getFileSize());
}

void Chip8_CoreInterface::copyFontToMemory(void* dest, const usz size) noexcept {
	std::memcpy(dest, std::data(sFontsData), size);
}

void Chip8_CoreInterface::copyColorsToCore(void* dest) noexcept {
	std::memcpy(dest, std::data(sBitColors), std::size(sBitColors) * sizeof(decltype(sBitColors)::value_type));
}

/*==================================================================*/

//f32 Chip8_CoreInterface::calcBuzzerTone() const noexcept {
//	return (160.0f + 8.0f * (
//		(mCurrentPC >> 1) + mStackTop + 1 & 0x3E
//	)) / ASB->getFrequency();
//}

//f32 Chip8_CoreInterface::calcBuzz8xTone(const u32 pitch) const noexcept {
//	return (160.0f + (
//		(0xFF - (pitch ? pitch : 0x80)) >> 3 << 4)
//	) / ASB->getFrequency();
//}
