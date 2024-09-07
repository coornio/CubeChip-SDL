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
#include "EmuInterface/EmuInterface.hpp"
#include "EmuInterface/GameFileChecker.hpp"

/*==================================================================*/
	#pragma region VM_Host Singleton Class
/*==================================================================*/

HomeDirManager* EmuHost::HDM{};
BasicVideoSpec* EmuHost::BVS{};
BasicAudioSpec* EmuHost::BAS{};

EmuHost::~EmuHost() noexcept = default;
EmuHost::EmuHost(const fsPath& gamePath) noexcept
	: Limiter{ std::make_unique<FrameLimiter>() }
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
		const auto totalElapsedTime{ currentFrameTime + Limiter->getRemainder() };

		const auto frameTimeDelta{ totalElapsedTime * 1.04f / Limiter->getFramespan() };
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
	BVS->resetWindow();
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
		BVS->changeTitle(HDM->getFileStem().c_str());
	}
}

/*==================================================================*/

void EmuHost::loadGameFile(const fsPath& gameFile) {
	BVS->raiseWindow();
	if (HDM->validateGameFile(gameFile)) {
		replaceCore();
	}
}

void EmuHost::pauseSystem(const bool state) const noexcept {
	if (state) {
		EmuInterface::addSystemState(EmuState::HIDDEN);
	} else {
		EmuInterface::subSystemState(EmuState::HIDDEN);
	}
}

/*==================================================================*/

void EmuHost::processFrame() {
	if (Limiter->checkTime()) {

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
		BAS->changeVolume(+15);
	}
	if (kb.isPressed(KEY(LEFT))) {
		BAS->changeVolume(-15);
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
		if (kb.isPressed(KEY(PAGEDOWN))) {
			BVS->changeFrameMultiplier(-1);
		}
		if (kb.isPressed(KEY(PAGEUP))) {
			BVS->changeFrameMultiplier(+1);
		}
	}
}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
