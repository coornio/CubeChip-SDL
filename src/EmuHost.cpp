/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_events.h>

#include "Assistants/BasicLogger.hpp"
#include "Assistants/BasicInput.hpp"
#include "Assistants/HomeDirManager.hpp"
#include "Assistants/BasicVideoSpec.hpp"
#include "Assistants/BasicAudioSpec.hpp"
#include "Assistants/DefaultConfig.hpp"

#include "EmuHost.hpp"
#include "Fonts/RobotoMono.hpp"
#include "Systems/EmuInterface.hpp"
#include "Systems/CoreRegistry.hpp"

/*==================================================================*/
	#pragma region VM_Host Singleton Class

EmuHost::EmuHost(const Path& gamePath) noexcept {
	EmuInterface::assignComponents(HDM, BVS);
	HDM->setValidator(CoreRegistry::validateProgram);
	CoreRegistry::loadProgramDB();

	if (!gamePath.empty()) { loadGameFile(gamePath); }
	if (!iGuest) { BVS->setMainWindowTitle(AppName, "Waiting for file..."); }
}

void EmuHost::StopEmuCoreThread::operator()(EmuInterface* ptr) noexcept {
	if (ptr) { ptr->stopWorker(); delete ptr; }
}

/*==================================================================*/

void EmuHost::discardCore() {
	iGuest.reset();
	
	BVS->setMainWindowTitle(AppName, "Waiting for file...");
	BVS->resetMainWindow();

	CoreRegistry::clearEligibleCores();

	HDM->clearCachedFileData();
}

void EmuHost::replaceCore() {
	iGuest.reset();
	iGuest.reset(CoreRegistry::constructCore());
	if (iGuest) {
		BVS->setMainWindowTitle(AppName, HDM->getFileStem());
		BVS->displayBuffer.resize(iGuest->getDisplaySize());
		toggleSystemLimiter();
		iGuest->startWorker();
	}
}

/*==================================================================*/

void EmuHost::loadGameFile(const Path& gameFile) {
	BVS->raiseMainWindow();
	blog.newEntry(BLOG::INFO, "Attempting to load: \"{}\"", gameFile.string());
	if (HDM->validateGameFile(gameFile)) {
		blog.newEntry(BLOG::INFO, "File has been accepted!");
		replaceCore();
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

	HDM->writeMainAppConfig(
		BAS->exportSettings().map(),
		BVS->exportSettings().map()
	);
}

bool EmuHost::initApplication(
	StrV overrideHome, StrV configName,
	bool forcePortable, StrV org, StrV app
) noexcept {
	HDM = HomeDirManager::initialize(overrideHome, configName, forcePortable, org, app);
	if (!HDM) { return false; }

	BasicAudioSpec::Settings BAS_settings;
	BasicVideoSpec::Settings BVS_settings;

	HDM->parseMainAppConfig(
		BAS_settings.map(),
		BVS_settings.map()
	);

	BAS = BasicAudioSpec::initialize(BAS_settings);
	if (!BAS) { return false; }

	BVS = BasicVideoSpec::initialize(BVS_settings);
	if (!BVS) { return false; }

	return true;
}

s32  EmuHost::processEvents(SDL_Event* event) noexcept {
	BVS->processInterfaceEvent(event);

	if (BVS->isMainWindowID(event->window.windowID)) {
		switch (event->type) {
			case SDL_EVENT_QUIT:
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				return SDL_APP_SUCCESS;

			case SDL_EVENT_DROP_FILE:
				loadGameFile(event->drop.data);
				break;

			case SDL_EVENT_WINDOW_MINIMIZED:
				hideMainWindow(true);
				break;

			case SDL_EVENT_WINDOW_RESTORED:
				hideMainWindow(false);
				break;

			case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
			case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
				BVS->scaleInterface(AppFontData_Roboto_Mono);
				break;
		}
	}

	return SDL_APP_CONTINUE;
}

/*==================================================================*/

void EmuHost::processFrame() {
	if (!BVS->isSuccessful())
		[[unlikely]] { return; }

	checkForHotkeys();

	if (iGuest && mFrameStat) {
		BVS->renderPresent(iGuest->copyOverlayData().c_str());
	} else {
		BVS->renderPresent(nullptr);
	}
}

void EmuHost::checkForHotkeys() {
	static BasicKeyboard Input;
	Input.updateStates();

	if (Input.isPressed(KEY(UP))) {
		BAS->addGlobalGain(+15);
	}
	if (Input.isPressed(KEY(DOWN))) {
		BAS->addGlobalGain(-15);
	}
	if (Input.isPressed(KEY(RIGHT))) {
		BVS->rotateViewport(+1);
	}
	if (Input.isPressed(KEY(LEFT))) {
		BVS->rotateViewport(-1);
	}
	if (Input.isPressed(KEY(F9))) {
		blog.newEntry(BLOG::INFO, "Attempting to load ProgramDB!");
		CoreRegistry::loadProgramDB();
	}
	if (Input.isPressed(KEY(F1))) {
		BVS->toggleUsingScanlines();
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

		if (Input.isPressed(KEY(F11))) {
			mFrameStat = !mFrameStat;
		}
		if (Input.isPressed(KEY(F12))) {
			mUnlimited = !mUnlimited;
			toggleSystemLimiter();
		}
	}
}

void EmuHost::toggleSystemLimiter() noexcept {
	if (!iGuest) { return; }
	if (mUnlimited) {
		iGuest->addSystemState(EmuState::BENCH);
	} else {
		iGuest->subSystemState(EmuState::BENCH);
	}
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
