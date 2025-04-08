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
#include "Systems/CoreRegistry.hpp"

/*==================================================================*/
	#pragma region VM_Host Singleton Class

EmuHost::~EmuHost() noexcept = default;
EmuHost::EmuHost(const Path& gamePath) noexcept {
	EmuInterface::assignComponents(HDM, BVS);
	HDM->setValidator(CoreRegistry::validateProgram);
	CoreRegistry::loadProgramDB();

	if (!gamePath.empty()) { loadGameFile(gamePath); }
}

void EmuHost::StopEmuCoreThread::operator()(EmuInterface* ptr) noexcept {
	if (ptr) { ptr->stopWorker(); delete ptr; }
}

/*==================================================================*/

void EmuHost::discardCore() {
	iGuest.reset();
	BVS->resetMainWindow();
	CoreRegistry::clearEligibleCores();

	HDM->clearCachedFileData();
}

void EmuHost::replaceCore() {
	iGuest.reset();
	iGuest.reset(CoreRegistry::constructCore());
	if (iGuest) {
		BVS->setMainWindowTitle(HDM->getFileStem().c_str());
		BVS->displayBuffer.resize(iGuest->getDisplaySize());
		iGuest->startWorker();
	}
}

/*==================================================================*/

void EmuHost::loadGameFile(const Path& gameFile) {
	BVS->raiseMainWindow();
	blog.newEntry(BLOG::INFO, "Attempting to load: \"{}\"", gameFile.string());
	if (HDM->validateGameFile(gameFile)) {
		replaceCore();
		blog.newEntry(BLOG::INFO, "File has been accepted!");
	} else {
		blog.newEntry(BLOG::INFO, "Path has been rejected!");
	}
}

void EmuHost::hideMainWindow(bool state) noexcept {
	if (!iGuest) { return; }

	if (state) {
		iGuest->addSystemState(EmuState::HIDDEN);
	} else {
		iGuest->subSystemState(EmuState::HIDDEN);
	}
}

void EmuHost::pauseSystem(bool state) noexcept {
	if (!iGuest) { return; }

	if (state) {
		iGuest->addSystemState(EmuState::PAUSED);
	} else {
		iGuest->subSystemState(EmuState::PAUSED);
	}
}

void EmuHost::quitApplication() noexcept {
	iGuest.reset();
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
	if (Input.isPressed(KEY(UP))) {
		BVS->rotateViewport(+1);
	}
	if (Input.isPressed(KEY(DOWN))) {
		BVS->rotateViewport(-1);
	}
	if (Input.isPressed(KEY(F1))) {
		blog.newEntry(BLOG::INFO, "Attempting to load ProgramDB!");
		CoreRegistry::loadProgramDB();
	}
	if (Input.isPressed(KEY(F2))) {
		BVS->toggleIntegerScaling();
	}
	if (Input.isPressed(KEY(F3))) {
		BVS->cycleViewportScaleMode();
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
