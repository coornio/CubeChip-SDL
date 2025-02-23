/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Assistants/BasicLogger.hpp"
#include "Assistants/BasicInput.hpp"
#include "Assistants/HomeDirManager.hpp"
#include "Assistants/BasicVideoSpec.hpp"
#include "Assistants/BasicAudioSpec.hpp"

#include "EmuHost.hpp"
#include "Systems/EmuInterface.hpp"
#include "Systems/GameFileChecker.hpp"

/*==================================================================*/
	#pragma region VM_Host Singleton Class

EmuHost::~EmuHost() noexcept = default;
EmuHost::EmuHost(const Path& gamePath) noexcept {
	EmuInterface::assignComponents(HDM, BVS);
	HDM->setValidator(GameFileChecker::validate);

	if (!gamePath.empty()) { loadGameFile(gamePath); }
}

void EmuHost::StopEmuCoreThread::operator()(EmuInterface* ptr) const noexcept {
	if (ptr) { ptr->stopWorker(); delete ptr; }
}

/*==================================================================*/

void EmuHost::discardCore() {
	iGuest.reset();
	BVS->resetMainWindow();
	GameFileChecker::deleteGameCore();

	HDM->clearCachedFileData();
}

void EmuHost::replaceCore() {
	iGuest.reset(GameFileChecker::constructCore());
	if (iGuest) {
		BVS->setMainWindowTitle(HDM->getFileStem().c_str());
		BVS->displayBuffer.resize(iGuest->getDisplaySize());
		iGuest->startWorker();
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
	if (iGuest) { discardCore(); }
}

bool EmuHost::isMainWindow(u32 windowID) const noexcept {
	return windowID == BVS->getMainWindowID();
}

/*==================================================================*/

void EmuHost::processFrame() {
	if (!BVS->isSuccessful())
		[[unlikely]] { return; }

	checkForHotkeys();

	if (iGuest && mFrameStat) {
		BVS->renderPresent(iGuest->fetchStatistics().c_str());
	} else {
		BVS->renderPresent(nullptr);
	}
}

void EmuHost::checkForHotkeys() {
	static BasicKeyboard Input;
	Input.updateStates();

	if (Input.isPressed(KEY(RIGHT))) {
		BAS->addGlobalGain(+15);
	}
	if (Input.isPressed(KEY(LEFT))) {
		BAS->addGlobalGain(-15);
	}

	if (iGuest) {
		if (Input.isPressed(KEY(ESCAPE))) {
			discardCore();
			return;
		}
		if (Input.isPressed(KEY(BACKSPACE))) {
			replaceCore();
			return;
		}

		if (Input.isPressed(KEY(RSHIFT))) {
			mFrameStat = !mFrameStat;
			if (!mFrameStat) { mUnlimited = false; }
		}
		if (Input.isPressed(KEY(F11))) {
			if (mFrameStat) { mUnlimited = !mUnlimited; }
		}
	}
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
