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
#include "Assistants/DefaultConfig.hpp"

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
	else { BVS->setMainWindowTitle(AppName, "Waiting for file..."); }
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

	auto BAS_settings{ BAS->exportSettings() };
	config::set(getAppConfig(), "Audio.f_Volume", BAS_settings.globalGain);
	config::set(getAppConfig(), "Audio.b_Muted",  BAS_settings.isMuted);

	auto BVS_settings{ BVS->exportSettings() };
	config::set(getAppConfig(), "Window.Position.i_X",       BVS_settings.windowPosX);
	config::set(getAppConfig(), "Window.Position.i_Y",       BVS_settings.windowPosY);
	config::set(getAppConfig(), "Window.Size.i_X",           BVS_settings.windowSizeX);
	config::set(getAppConfig(), "Window.Size.i_Y",           BVS_settings.windowSizeY);
	config::set(getAppConfig(), "Viewport.i_ScaleMode",      BVS_settings.viewportScaleMode);
	config::set(getAppConfig(), "Viewport.b_IntegerScaling", BVS_settings.integerScaling);
	config::set(getAppConfig(), "Viewport.b_UsingScanlines", BVS_settings.usingScanlines);

	HDM->writeMainConfig();
}

bool EmuHost::initApplication(const char*) noexcept {
	HDM = HomeDirManager::initialize(nullptr, nullptr, nullptr, AppName);
	if (!HDM) { return false; }

	BasicAudioSpec::Settings BAS_settings{
		config::get<f32>(getAppConfig(),  "Audio.f_Volume"),
		config::get<bool>(getAppConfig(), "Audio.b_Muted"),
	};
	BAS = BasicAudioSpec::create(BAS_settings);
	if (!HDM) { return false; }

	BasicVideoSpec::Settings BVS_settings{
		config::get<s32> (getAppConfig(), "Window.Position.i_X"),
		config::get<s32> (getAppConfig(), "Window.Position.i_Y"),
		config::get<s32> (getAppConfig(), "Window.Size.i_X"),
		config::get<s32> (getAppConfig(), "Window.Size.i_Y"),
		config::get<s32> (getAppConfig(), "Viewport.i_ScaleMode"),
		config::get<bool>(getAppConfig(), "Viewport.b_IntegerScaling"),
		config::get<bool>(getAppConfig(), "Viewport.b_UsingScanlines"),
	};
	BVS = BasicVideoSpec::create(BVS_settings);
	if (!BVS) { return false; }

	return true;
}

SDL_AppResult EmuHost::processEvents(SDL_Event* event) noexcept {
	BVS->processInterfaceEvent(event);

	if (event->window.windowID == BVS->getMainWindowID()) {
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
				BVS->scaleInterface(AppFontData);
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
