/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <iostream>
#include <format>
#include <cmath>

#include "Assistants/BasicLogger.hpp"
#include "Assistants/BasicInput.hpp"
#include "Assistants/FrameLimiter.hpp"
#include "Assistants/HomeDirManager.hpp"
#include "Assistants/BasicVideoSpec.hpp"
#include "Assistants/BasicAudioSpec.hpp"

#include "EmuHost.hpp"
#include "Systems/EmuInterface.hpp"
#include "Systems/GameFileChecker.hpp"

/*==================================================================*/
	#pragma region VM_Host Singleton Class

EmuHost::~EmuHost() noexcept = default;
EmuHost::EmuHost(const Path& gamePath) noexcept
	: Limiter{ std::make_unique<FrameLimiter>(60.0f, true, true) }
{
	static BasicKeyboard sInput;
	Input = &sInput;

	EmuInterface::assignComponents(HDM, BVS);
	HDM->setValidator(GameFileChecker::validate);

	if (!gamePath.empty()) { loadGameFile(gamePath); }
}

/*==================================================================*/

const StrV EmuHost::getStats() const {
	if (!mFrameStat || iGuest->isSystemStopped())
		[[unlikely]] { return ""; }

	static Str stats{};
	if (Limiter->getValidFrameCounter() & 0x1) [[likely]] {
		const auto currentFrameTime{ Limiter->getElapsedMicrosSince() / 1000.0f };

		if (mUnlimited) {
			const auto frameTimeDelta{ currentFrameTime * 1.04f / Limiter->getFramespan() };
			const auto workCycleDelta{ 1e5f * std::sin((1 - frameTimeDelta) * 1.5707963f) };

			stats = std::format(
				" ::   MIPS:{:8.2f}\n"
				"Time Since:{:9.3f} ms\n"
				"Frame Work:{:9.3f} ms\n",
				iGuest->addCPF(static_cast<s32>(workCycleDelta))
					* iGuest->getFramerate() / 1'000'000.0f,
				Limiter->getElapsedMillisLast(), currentFrameTime
			);
		} else {
			stats = std::format(
				"Time Since:{:9.3f} ms\n"
				"Frame Work:{:9.3f} ms\n",
				Limiter->getElapsedMillisLast(), currentFrameTime
			);
		}
	}
	
	return stats;
}

/*==================================================================*/

void EmuHost::discardCore() {
	iGuest.reset();
	BVS->resetMainWindow();
	GameFileChecker::deleteGameCore();

	Limiter->setLimiter(30.0f);
	HDM->clearCachedFileData();
}

void EmuHost::replaceCore() {
	iGuest = GameFileChecker::initGameCore();

	if (iGuest) {
		Limiter->setLimiter(iGuest->getFramerate());
		BVS->setMainWindowTitle(HDM->getFileStem().c_str());
	}
}

/*==================================================================*/

void EmuHost::loadGameFile(const Path& gameFile) {
	BVS->raiseMainWindow();
	blog.newEntry(BLOG::INFO, "Attempting to access: \"{}\"", gameFile.string());
	if (HDM->validateGameFile(gameFile)) {
		replaceCore();
		blog.newEntry(BLOG::INFO, "File has been accepted!");
	} else {
		blog.newEntry(BLOG::INFO, "Path has been rejected!");
	}
}

void EmuHost::pauseSystem(bool state) const noexcept {
	if (state) {
		EmuInterface::addSystemState(EmuState::HIDDEN);
	} else {
		EmuInterface::subSystemState(EmuState::HIDDEN);
	}
}

void EmuHost::quitApplication() noexcept {
	discardCore();
}

bool EmuHost::isMainWindow(u32 windowID) const noexcept {
	return windowID == BVS->getMainWindowID();
}

/*==================================================================*/

void EmuHost::processFrame() {
	if (Limiter->checkTime()) {
		if (!BVS->isSuccessful())
			[[unlikely]] { return; }

		Input->updateStates();
		checkForHotkeys();

		if (iGuest) [[likely]] {
			iGuest->processFrame();
			BVS->renderPresent(getStats().data());
		} else {
			BVS->renderPresent(nullptr);
		}
	}
}

void EmuHost::checkForHotkeys() {
	if (Input->isPressed(KEY(RIGHT))) {
		BAS->addGlobalGain(+15);
	}
	if (Input->isPressed(KEY(LEFT))) {
		BAS->addGlobalGain(-15);
	}

	if (iGuest) {
		if (Input->isPressed(KEY(ESCAPE))) {
			discardCore();
			return;
		}
		if (Input->isPressed(KEY(BACKSPACE))) {
			replaceCore();
			return;
		}

		if (Input->isPressed(KEY(RSHIFT))) {
			mFrameStat = !mFrameStat;
			if (!mFrameStat) { mUnlimited = false; }
		}
		if (Input->isPressed(KEY(F11))) {
			if (mFrameStat) { mUnlimited = !mUnlimited; }
		}
	}
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
