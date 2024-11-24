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
	sSavestatePath = HDM->addSystemDir("savestate", "CHIP8");
	if (!sSavestatePath) { setCoreState(EmuState::FATAL); }
	sPermaRegsPath = HDM->addSystemDir("permaRegs", "CHIP8");
	if (!sPermaRegsPath) { setCoreState(EmuState::FATAL); }

	ASB->resumeStreams();
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
				mInterrupt  = Interrupt::CLEAR;
				mActiveCPF  = std::abs(mActiveCPF);
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

	mAudioTimer[index] = duration & 0xFF;
	mPhaseStep[index] = (160.0f + (tone ? tone
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

void Chip8_CoreInterface::triggerCritError(const Str& msg) noexcept {
	blog.newEntry(BLOG::WARN, msg);
	triggerInterrupt(Interrupt::ERROR);
}

/*==================================================================*/

bool Chip8_CoreInterface::setPermaRegs(const u32 X) noexcept {
	const auto path{ *sPermaRegsPath / HDM->getFileSHA1() };
	std::error_code error_code;

	const bool fileExists{ doesFileExist(path, error_code) };
	if (error_code) {
		blog.newEntry(BLOG::ERROR, "Path is ineligible: \"{}\" [{}]",
			path.string(), error_code.message()
		);
		return false;
	}

	if (fileExists) {
		auto regsData{ readFileData(path, error_code) };

		if (error_code) {
			blog.newEntry(BLOG::ERROR, "File IO error: \"{}\" [{}]",
				path.string(), error_code.message()
			);
			return false;
		}
		if (regsData.size() > mRegisterV.size()) {
			blog.newEntry(BLOG::ERROR, "File is too large: \"{}\" [{} bytes]",
				path.string(), regsData.size()
			);
			return false;
		}

		regsData.resize(mRegisterV.size());
		std::copy_n(mRegisterV.begin(), X, regsData.begin());

		if (!writeFileData(path, regsData, error_code)) {
			blog.newEntry(BLOG::ERROR, "File IO error: \"{}\" [{}]",
				path.string(), error_code.message()
			);
			return false;
		} else {
			return true;
		}
	} else {
		char regsData[sizeof(mRegisterV)]{};
		std::copy_n(mRegisterV.begin(), X, regsData);

		if (!writeFileData(path, regsData, error_code)) {
			blog.newEntry(BLOG::ERROR, "File IO error: \"{}\" [{}]",
				path.string(), error_code.message()
			);
			return false;
		} else {
			return true;
		}
	}
}

bool Chip8_CoreInterface::getPermaRegs(const u32 X) noexcept {
	const auto path{ *sPermaRegsPath / HDM->getFileSHA1() };
	std::error_code error_code;

	const bool fileExists{ doesFileExist(path, error_code) };
	if (error_code) {
		blog.newEntry(BLOG::ERROR, "Path is ineligible: \"{}\" [{}]",
			path.string(), error_code.message()
		);
		return false;
	}

	if (fileExists) {
		auto regsData{ readFileData(path, error_code) };

		if (error_code) {
			blog.newEntry(BLOG::ERROR, "File IO error: \"{}\" [{}]",
				path.string(), error_code.message()
			);
			return false;
		}
		if (regsData.size() > mRegisterV.size()) {
			blog.newEntry(BLOG::ERROR, "File is too large: \"{}\" [{} bytes]",
				path.string(), regsData.size()
			);
			return false;
		}

		regsData.resize(mRegisterV.size());
		std::copy_n(regsData.begin(), X, mRegisterV.begin());
		return true;
	} else {
		std::fill_n(mRegisterV.begin(), X, u8{});
		return true;
	}
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
	u8* dest, const u32 offset, const usz size
) noexcept {
	std::copy_n(
		std::execution::unseq,
		sFontsData.begin(), size, dest + offset
	);
}

void Chip8_CoreInterface::copyColorsToCore(
	u32* dest, const usz size
) noexcept {
	std::copy_n(
		std::execution::unseq,
		sBitColors.begin(), size, dest
	);
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
