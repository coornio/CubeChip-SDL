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
	: Limiter{ std::make_unique<FrameLimiter>(60.0f, true, false) }
{
	EmuInterface::assignComponents(HDM, BVS, BAS);
	HDM->setValidator(GameFileChecker::validate);

	if (!gamePath.empty()) { loadGameFile(gamePath); }
}

/*==================================================================*/

void EmuHost::toggleUnlimited() {
	unlimitedMode = !unlimitedMode;
	if (!unlimitedMode) { std::cout << "\n\n\n"; }
}

void EmuHost::printStatistics() const {
	if (!unlimitedMode || iGuest->isSystemStopped()) { return; }

	if (Limiter->getValidFrameCounter() & 0x1) {
		const auto currentFrameTime{ Limiter->getElapsedMicrosSince() / 1000.0f };

		const auto frameTimeDelta{ currentFrameTime * 1.04f / Limiter->getFramespan() };
		const auto workCycleDelta{ 1e5f * std::sin((1 - frameTimeDelta) * 1.5707963f) };

		std::cout << std::format(
			"Frame time:{:9.4f} ms\n"
			"Time since:{:9.4f} ms\n"
			" ::   MIPS:{:8.2f}\33[3F",
			currentFrameTime,
			Limiter->getElapsedMillisLast(),
			iGuest->changeCPF(static_cast<s32>(workCycleDelta))
				* iGuest->getFramerate() / 1'000'000.0f
		) << std::endl;
	}
}

/*==================================================================*/

void EmuHost::discardCore() {
	binput::kb.updateCopy();
	binput::mb.updateCopy();

	iGuest.reset();
	BVS->resetMainWindow();
	GameFileChecker::deleteGameCore();

	Limiter->setLimiter(30.0f);
	HDM->clearCachedFileData();
}

void EmuHost::replaceCore() {
	binput::kb.updateCopy();
	binput::mb.updateCopy();

	iGuest = GameFileChecker::initGameCore();

	if (iGuest) {
		Limiter->setLimiter(iGuest->getFramerate());
		BVS->setMainWindowTitle(HDM->getFileStem());
	}
}

/*==================================================================*/

void EmuHost::loadGameFile(const Path& gameFile) {
	BVS->raiseMainWindow();
	blog.newEntry(BLOG::INFO, "Attempting to access file: \"{}\"", gameFile.string());
	if (HDM->validateGameFile(gameFile)) {
		replaceCore();
		blog.newEntry(BLOG::INFO, "File has been accepted!");
	} else {
		blog.newEntry(BLOG::INFO, "File has been rejected!");
	}
}

void EmuHost::pauseSystem(const bool state) const noexcept {
	if (state) {
		EmuInterface::addSystemState(EmuState::HIDDEN);
	} else {
		EmuInterface::subSystemState(EmuState::HIDDEN);
	}
}

void EmuHost::quitApplication() noexcept {
	discardCore();
}

bool EmuHost::isMainWindow(const u32 windowID) const noexcept {
	return windowID == BVS->getMainWindowID();
}

/*==================================================================*/

void EmuHost::processFrame() {
	if (Limiter->checkTime()) {
		if (BVS->getErrorState())
			[[unlikely]] { return; }

		checkForHotkeys();

		if (iGuest) {
			iGuest->processFrame();
			printStatistics();
		}

		BVS->renderPresent();

		binput::kb.updateCopy();
		binput::mb.updateCopy();
	}
}

void EmuHost::checkForHotkeys() {
	using namespace binput;

	if (kb.isPressed(KEY(RIGHT))) {
		BAS->changeGlobalVolume(+15);
	}
	if (kb.isPressed(KEY(LEFT))) {
		BAS->changeGlobalVolume(-15);
	}

	if (iGuest) {
		if (kb.isPressed(KEY(ESCAPE))) {
			discardCore();
			return;
		}
		if (kb.isPressed(KEY(BACKSPACE))) {
			replaceCore();
			return;
		}

		if (kb.isPressed(KEY(RSHIFT))) {
			toggleUnlimited();
		}
	}
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
