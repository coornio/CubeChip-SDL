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

#include "FrontendHost.hpp"
#include "Fonts/RobotoMono.hpp"
#include "Systems/SystemsInterface.hpp"
#include "Systems/CoreRegistry.hpp"

/*==================================================================*/

FrontendHost::FrontendHost(const Path& gamePath) noexcept {
	SystemsInterface::assignComponents(HDM, BVS);
	HDM->setValidator(CoreRegistry::validateProgram);
	CoreRegistry::loadProgramDB();

	if (!gamePath.empty()) { loadGameFile(gamePath); }
	if (!mSystemCore) { BVS->setMainWindowTitle(AppName, "Waiting for file..."); }
}

void FrontendHost::StopEmuCoreThread::operator()(SystemsInterface* ptr) noexcept {
	if (ptr) { ptr->stopWorker(); delete ptr; }
}

/*==================================================================*/

void FrontendHost::discardCore() {
	mSystemCore.reset();
	
	BVS->setMainWindowTitle(AppName, "Waiting for file...");
	BVS->resetMainWindow();

	CoreRegistry::clearEligibleCores();

	HDM->clearCachedFileData();
}

void FrontendHost::replaceCore() {
	mSystemCore.reset();
	mSystemCore.reset(CoreRegistry::constructCore());
	if (mSystemCore) {
		BVS->setMainWindowTitle(AppName, HDM->getFileStem());
		BVS->displayBuffer.resize(mSystemCore->getDisplaySize());
		toggleSystemLimiter();
		mSystemCore->startWorker();
	}
}

/*==================================================================*/

void FrontendHost::loadGameFile(const Path& gameFile) {
	BVS->raiseMainWindow();
	blog.newEntry(BLOG::INFO, "Attempting to load: \"{}\"", gameFile.string());
	if (HDM->validateGameFile(gameFile)) {
		blog.newEntry(BLOG::INFO, "File has been accepted!");
		replaceCore();
	} else {
		blog.newEntry(BLOG::INFO, "Path has been rejected!");
	}
}

void FrontendHost::hideMainWindow(bool state) noexcept {
	if (!mSystemCore) { return; }

	if (state) {
		mSystemCore->addSystemState(EmuState::HIDDEN);
	} else {
		mSystemCore->subSystemState(EmuState::HIDDEN);
	}
}

void FrontendHost::pauseSystem(bool state) noexcept {
	if (!mSystemCore) { return; }

	if (state) {
		mSystemCore->addSystemState(EmuState::PAUSED);
	} else {
		mSystemCore->subSystemState(EmuState::PAUSED);
	}
}

void FrontendHost::quitApplication() noexcept {
	mSystemCore.reset();

	HDM->writeMainAppConfig(
		BAS->exportSettings().map(),
		BVS->exportSettings().map()
	);
}

bool FrontendHost::initApplication(
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

s32  FrontendHost::processEvents(SDL_Event* event) noexcept {
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

void FrontendHost::processFrame() {
	if (!BVS->isSuccessful())
		[[unlikely]] { return; }

	checkForHotkeys();

	if (mSystemCore && mFrameStat) {
		BVS->renderPresent(mSystemCore->copyOverlayData().c_str());
	} else {
		BVS->renderPresent(nullptr);
	}
}

void FrontendHost::checkForHotkeys() {
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

	if (mSystemCore) {
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

void FrontendHost::toggleSystemLimiter() noexcept {
	if (!mSystemCore) { return; }
	if (mUnlimited) {
		mSystemCore->addSystemState(EmuState::BENCH);
	} else {
		mSystemCore->subSystemState(EmuState::BENCH);
	}
}

/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
