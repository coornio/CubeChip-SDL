/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <iostream>
#include <iomanip>

#include "../Assistants/BasicLogger.hpp"
#include "../Assistants/BasicInput.hpp"
#include "../Assistants/FrameLimiter.hpp"

#include "HomeDirManager.hpp"
#include "BasicVideoSpec.hpp"
#include "BasicAudioSpec.hpp"

#include "EmuHost.hpp"
#include "../GuestClass/EmuCores/EmuCores.hpp"
#include "../GuestClass/GameFileChecker.hpp"

/*==================================================================*/
	#pragma region VM_Host Singleton Class
/*==================================================================*/

HomeDirManager* EmuHost::HDM{};
BasicVideoSpec* EmuHost::BVS{};
BasicAudioSpec* EmuHost::BAS{};

EmuHost::~EmuHost() noexcept = default;
EmuHost::EmuHost(const std::filesystem::path& gamePath) noexcept
	: Limiter{ std::make_unique<FrameLimiter>() }
{
	EmuInterface::assignComponents(HDM, BVS, BAS);
	HDM->setValidator(GameFileChecker::validate);

	if (!gamePath.empty()) { loadGameFile(gamePath); }
}

/*==================================================================*/

void EmuHost::toggleUnlimited() {
	if (unlimitedMode) {
		unlimitedMode = false;
		std::cout << "\33[1;1H\33[3J" << std::endl;
	} else {
		unlimitedMode = true;
		std::cout << "\33[1;1H\33[2J"
				  << "Frame time:\33[10Cms\n"
				  << "Time since:\33[10Cms\n"
				  << " ::   MIPS:"
				  << std::endl;
	}
}

void EmuHost::printStatistics() const {
	if (!unlimitedMode) { return; }

	if (Limiter->getValidFrameCounter() & 0x1) {
		std::cout
			<< "\33[1;12H" << std::setfill(' ') << std::setw(9)
			<< std::right << std::fixed << std::setprecision(4)
			<< Limiter->getElapsedMicrosSince() / 1e3

			<< "\33[2;12H" << std::setfill(' ') << std::setw(9)
			<< std::right << std::fixed << std::setprecision(4)
			<< Limiter->getElapsedMillisLast()

			<< "\33[3;12H" << std::setfill(' ') << std::setw(8)
			<< std::right << std::fixed << std::setprecision(2)
			<< iGuest->changeCPF(Limiter->isKeepingPace() ? +5000 : -5000) * iGuest->fetchFramerate() / 1e6
			<< std::endl;
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
		Limiter->setLimiter(iGuest->fetchFramerate());
		BVS->changeTitle(HDM->getFileStem().c_str());
	}
}

/*==================================================================*/

void EmuHost::loadGameFile(const std::filesystem::path& gameFile) {
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
