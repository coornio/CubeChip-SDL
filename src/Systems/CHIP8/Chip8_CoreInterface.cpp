/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../Assistants/FrameLimiter.hpp"
#include "../../Assistants/BasicInput.hpp"
#include "../../Assistants/BasicLogger.hpp"
#include "../../Assistants/HomeDirManager.hpp"
#include "../../Assistants/BasicVideoSpec.hpp"
#include "../../Assistants/BasicAudioSpec.hpp"
#include "../../Assistants/SimpleFileIO.hpp"

#include "Chip8_CoreInterface.hpp"

/*==================================================================*/

Chip8_CoreInterface::Chip8_CoreInterface() noexcept {
	if (const auto* path{ HDM->addSystemDir("savestate", "CHIP8") })
		{ sSavestatePath = *path / HDM->getFileSHA1(); }

	if (const auto* path{ HDM->addSystemDir("permaRegs", "CHIP8") })
		{ sPermaRegsPath = *path / HDM->getFileSHA1(); }

	mAudio.addAudioStream(STREAM::CHANN0, AUDIOFORMAT::S16, 1, 48'000);
	mAudio.addAudioStream(STREAM::CHANN1, AUDIOFORMAT::S16, 1, 48'000);
	mAudio.addAudioStream(STREAM::CHANN2, AUDIOFORMAT::S16, 1, 48'000);
	mAudio.addAudioStream(STREAM::CHANN3, AUDIOFORMAT::S16, 1, 48'000);

	mAudio.resumeStreams();
	loadPresetBinds();
}

/*==================================================================*/

void Chip8_CoreInterface::updateKeyStates() {
	if (!std::size(mCustomBinds)) { return; }

	Input->updateStates();

	mKeysPrev = mKeysCurr;
	mKeysCurr = 0;

	for (const auto& mapping : mCustomBinds) {
		if (Input->areAnyHeld(mapping.key, mapping.alt)) {
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

bool Chip8_CoreInterface::keyPressed(u8* returnKey) noexcept {
	if (!std::size(mCustomBinds)) { return false; }

	const auto mTickCurr{ static_cast<u32>(Pacer->getValidFrameCounter()) };
	if (mTickCurr >= mTickLast + mTickSpan)
		[[unlikely]] { mKeysPrev &= ~mKeysLoop; }

	/**/const auto pressKeys{ mKeysCurr & ~mKeysPrev };
	if (pressKeys) {
		const auto pressDiff{ pressKeys & ~mKeysLoop };
		const auto validKeys{ pressDiff ? pressDiff : mKeysLoop };

		mKeysLock |= validKeys;
		mTickLast  = mTickCurr;
		mTickSpan  = validKeys != mKeysLoop ? 20 : 5;
		mKeysLoop  = validKeys & ~(validKeys - 1);
		*returnKey = std::countr_zero(mKeysLoop) & 0xFF;
		//mKeyPitch = mKeysLoop ? std::min(mKeyPitch + 8, 80u) : 0;
	}
	return pressKeys;
}

bool Chip8_CoreInterface::keyHeld_P1(u32 keyIndex) const noexcept {
	return mKeysCurr & ~mKeysLock & 0x01 << (keyIndex & 0xF);
}

bool Chip8_CoreInterface::keyHeld_P2(u32 keyIndex) const noexcept {
	return mKeysCurr & ~mKeysLock & 0x10 << (keyIndex & 0xF);
}

/*==================================================================*/

void Chip8_CoreInterface::handlePreFrameInterrupt() noexcept {
	switch (mInterrupt)
	{
		case Interrupt::FRAME:
			mInterrupt = Interrupt::CLEAR;
			mTargetCPF = std::abs(mTargetCPF);
			return;

		case Interrupt::SOUND:
			if (mAudioTimer[0]) { return; }
			if (mAudioTimer[1]) { return; }
			if (mAudioTimer[2]) { return; }
			if (mAudioTimer[3]) { return; }
			mInterrupt = Interrupt::FINAL;
			mTargetCPF = 0;
			return;

		case Interrupt::DELAY:
			if (!mDelayTimer) {
				mInterrupt = Interrupt::CLEAR;
				mTargetCPF = std::abs(mTargetCPF);
			}
			return;

		default:
			break;
	}
}

void Chip8_CoreInterface::handleEndFrameInterrupt() noexcept {
	switch (mInterrupt)
	{
		case Interrupt::INPUT:
			if (keyPressed(mInputReg)) {
				mInterrupt = Interrupt::CLEAR;
				mTargetCPF = std::abs(mTargetCPF);
				startAudioAtChannel(STREAM::BUZZER, 2);
			}
			return;

		case Interrupt::ERROR:
			addSystemState(EmuState::FATAL);
			mTargetCPF = 0;
			return;

		case Interrupt::FINAL:
			setSystemState(EmuState::HALTED);
			mTargetCPF = 0;
			return;

		default:
			break;
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

void Chip8_CoreInterface::performProgJump(u32 next) noexcept {
	const auto NNN{ next & 0xFFF };
	if (mCurrentPC - 2u != NNN) [[likely]] {
		mCurrentPC = NNN;
	} else {
		triggerInterrupt(Interrupt::SOUND);
	}
}

/*==================================================================*/

void Chip8_CoreInterface::mainSystemLoop() {
	if (Pacer->checkTime()) {
		if (!isSystemRunning())
			[[unlikely]] { return; }

		updateKeyStates();

		handleTimerTick();
		handlePreFrameInterrupt();
		instructionLoop();
		handleEndFrameInterrupt();

		renderAudioData();
		renderVideoData();
		pushOverlayData();
	}
}

Str* Chip8_CoreInterface::makeOverlayData() {
	const auto currentFrameTime{ Pacer->getElapsedMicrosSince() / 1000.0f };
	const auto frameTimeBias{ currentFrameTime * 1.025f / Pacer->getFramespan() };
	const auto workCycleBias{ 1e5f * std::sin((1 - frameTimeBias) * 1.5707963f) };

	if (mInterrupt == Interrupt::CLEAR) [[likely]]
		{ mTargetCPF += static_cast<s32>(workCycleBias); }

	*getOverlayDataBuffer() = fmt::format(
		" ::  MIPS:{:8.2f}\n{}",
		mTargetCPF * getSystemFramerate() / 1'000'000.0f,
		*SystemsInterface::makeOverlayData()
	);
	return getOverlayDataBuffer();
}

void Chip8_CoreInterface::pushOverlayData() {
	if (getSystemState() & EmuState::BENCH) [[likely]]
		{ saveOverlayData(makeOverlayData()); }
	else { SystemsInterface::pushOverlayData(); }
}

/*==================================================================*/

void Chip8_CoreInterface::startAudio(s32 duration, s32 tone) noexcept {
	static auto index{ 0 };
	startAudioAtChannel(index, duration, tone);
	if (duration) { ++index %= STREAM::COUNT; }
}

void Chip8_CoreInterface::startAudioAtChannel(s32 index, s32 duration, s32 tone) noexcept {
	if (index >= STREAM::COUNT) { return; }

	::assign_cast(mAudioTimer[index], duration);
	if (auto* stream{ mAudio.at(index) }) {
		mPhaseStep[index] = (sTonalOffset + (tone ? tone : 8 * (
			((mCurrentPC >> 1) + mStackTop + 1) & 0x3E
		))) / stream->getFreq();
	}
}

void Chip8_CoreInterface::pushSquareTone(s32 index) noexcept {
	if (auto* stream{ mAudio.at(index) }) {
		const auto samplesTotal{ stream->getNextBufferSize(getSystemFramerate()) };
		auto samplesBuffer{ ::allocate<s16>(samplesTotal).as_value().release() };

		if (mAudioTimer[index]) {
			std::for_each_n(EXEC_POLICY(unseq)
				samplesBuffer.get(), samplesTotal,
				[start = samplesBuffer.get(), phase = mAudioPhase[index], step = mPhaseStep[index]] \
				(auto& audioSample) noexcept {
					const auto val{ step * (&audioSample - start) + phase };
					const bool mask{ val - static_cast<int>(val) >= 0.5f };
					::assign_cast(audioSample, mask * 8192 - 4096);
				}
			);
			mAudioPhase[index] += mPhaseStep[index] * samplesTotal;
			mAudioPhase[index] -= static_cast<int>(mAudioPhase[index]);
		} else { mAudioPhase[index] = 0.0f; }

		stream->pushAudioData(samplesBuffer.get(), samplesTotal);
	}
}

void Chip8_CoreInterface::instructionError(u32 HI, u32 LO) {
	blog.newEntry(BLOG::INFO, "Unknown instruction: 0x{:04X}", HI << 8 | LO);
	triggerInterrupt(Interrupt::ERROR);
}

void Chip8_CoreInterface::triggerInterrupt(Interrupt type) noexcept {
	mInterrupt = type;
	mTargetCPF = -std::abs(mTargetCPF);
}

/*==================================================================*/

bool Chip8_CoreInterface::checkRegularFile(const Path& filePath) const noexcept {
	const auto fileRegular{ fs::is_regular_file(filePath) };
	if (!fileRegular) {
		blog.newEntry(BLOG::ERROR, "\"{}\" [{}]",
			filePath.string(), fileRegular.error().message());
		return false;
	}
	return fileRegular.value();
}

bool Chip8_CoreInterface::newPermaRegsFile(const Path& filePath) const noexcept {
	static constexpr char dataPadding[std::size(sPermRegsV)]{};
	const auto fileCreated{ ::writeFileData(filePath, dataPadding) };
	if (!fileCreated) {
		blog.newEntry(BLOG::ERROR, "\"{}\" [{}]",
			filePath.string(), fileCreated.error().message());
	}
	return fileCreated.value();
}

void Chip8_CoreInterface::setFilePermaRegs(u32 X) noexcept {
	auto fileData{ ::writeFileData(sPermaRegsPath, mRegisterV, X) };
	if (!fileData) {
		blog.newEntry(BLOG::ERROR, "File IO error: \"{}\" [{}]",
			sPermaRegsPath.string(), fileData.error().message());
	}
}

void Chip8_CoreInterface::getFilePermaRegs(u32 X) noexcept {
	auto fileData{ ::readFileData(sPermaRegsPath, X) };
	if (!fileData) {
		blog.newEntry(BLOG::ERROR, "File IO error: \"{}\" [{}]",
			sPermaRegsPath.string(), fileData.error().message());
	} else {
		std::copy_n(fileData.value().begin(), X, sPermRegsV.begin());
	}
}

void Chip8_CoreInterface::setPermaRegs(u32 X) noexcept {
	if (!sPermaRegsPath.empty()) {
		if (checkRegularFile(sPermaRegsPath)) { setFilePermaRegs(X); }
		else { sPermaRegsPath.clear(); }
	}
	std::copy_n(mRegisterV.begin(), X, sPermRegsV.begin());
}

void Chip8_CoreInterface::getPermaRegs(u32 X) noexcept {
	if (!sPermaRegsPath.empty()) {
		if (!checkRegularFile(sPermaRegsPath)) {
			if (!newPermaRegsFile(sPermaRegsPath)) { sPermaRegsPath.clear(); }
		}

		if (checkRegularFile(sPermaRegsPath)) { getFilePermaRegs(X); }
		else { sPermaRegsPath.clear(); }
	}
	std::copy_n(sPermRegsV.begin(), X, mRegisterV.begin());
}

/*==================================================================*/

void Chip8_CoreInterface::copyGameToMemory(void* dest) noexcept {
	std::memcpy(dest, HDM->getFileData(), HDM->getFileSize());
}

void Chip8_CoreInterface::copyFontToMemory(void* dest, ust size) noexcept {
	std::memcpy(dest, std::data(sFontsData), size);
}

void Chip8_CoreInterface::copyColorsToCore(void* dest) noexcept {
	std::memcpy(dest, std::data(sBitColors), std::size(sBitColors) * sizeof(decltype(sBitColors)::value_type));
}

/*==================================================================*/
