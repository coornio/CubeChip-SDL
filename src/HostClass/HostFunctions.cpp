/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <iostream>
#include <iomanip>

#include "HomeDirManager.hpp"
#include "BasicVideoSpec.hpp"
#include "BasicAudioSpec.hpp"

#include "../Assistants/BasicLogger.hpp"
#include "../Assistants/BasicInput.hpp"
#include "../Assistants/FrameLimiter.hpp"

#include "Host.hpp"
#include "../GuestClass/EmuCores/EmuCores.hpp"
#include "../GuestClass/GameFileChecker.hpp"

using namespace blogger;
using namespace bic;

/*------------------------------------------------------------------*/
/*  class  VM_Host                                                  */
/*------------------------------------------------------------------*/

VM_Host::VM_Host(const char* const filename)
	: HDM    { std::make_unique<HomeDirManager>("CubeChip") }
	, BVS    { std::make_unique<BasicVideoSpec>() }
	, BAS    { std::make_unique<BasicAudioSpec>() }
	, Limiter{ std::make_unique<FrameLimiter>() }
{
	HDM->setValidator(GameFileChecker::validate);
	loadGameFile(filename);
}

VM_Host* VM_Host::initialize(const char* const filename) {
	try {
		static VM_Host self(filename);
		return &self;
	} catch (...) {
		return nullptr;
	}
}

bool VM_Host::initGameCore() {
	iGuest = std::move(GameFileChecker::initializeCore(*HDM, *BVS, *BAS));
	return iGuest ? true : false;
}

SDL_AppResult VM_Host::runFrame() {
	using namespace bic;

	if (!Limiter->checkTime()) [[likely]] { return SDL_APP_CONTINUE; }

	if (kb.isPressed(KEY(RIGHT))) {
		BAS->changeVolume(+15);
	}
	if (kb.isPressed(KEY(LEFT))) {
		BAS->changeVolume(-15);
	}

	if (iGuest) {
		if (kb.isPressed(KEY(ESCAPE))) {
			replaceGuest(true);
			return SDL_APP_CONTINUE;
		}
		if (kb.isPressed(KEY(BACKSPACE))) {
			loadGameFile(HDM->getFilePath().c_str());
			return SDL_APP_CONTINUE;
		}
		if (kb.isPressed(KEY(RSHIFT))) {
			if (runBenchmark) {
				runBenchmark = false;
				BVS->changeTitle(HDM->getFileStem().c_str());
				std::cout << "\33[1;1H\33[3J" << std::endl;
			} else {
				runBenchmark = true;
				BVS->changeTitle(std::to_string(iGuest->fetchCPF()));
				std::cout << "\33[1;1H\33[2J"
					<< "Cycle time:    .    ms"
					<< "\nTime since last frame: "
				#ifndef SDL_PLATFORM_WIN32
					<< "\nCPF: "
				#endif
					<< std::endl;
			}
		}

		if (kb.isPressed(KEY(PAGEDOWN))){
			BVS->changeFrameMultiplier(-1);
		}
		if (kb.isPressed(KEY(PAGEUP))) {
			BVS->changeFrameMultiplier(+1);
		}

		if (runBenchmark) [[likely]] {
			if (kb.isPressed(KEY(UP))) {
				BVS->changeTitle(std::to_string(iGuest->changeCPF(+50'000)));
			}
			if (kb.isPressed(KEY(DOWN))){
				BVS->changeTitle(std::to_string(iGuest->changeCPF(-50'000)));
			}

			iGuest->processFrame();

			if (Limiter->getValidFrameCounter() & 0x1) {
				const auto micros{ Limiter->getElapsedMicrosSince() };
				std::cout
					<< "\33[1;12H"
					<< std::setfill(' ') << std::setw(4) << micros / 1000
					<< "\33[1C"
					<< std::setfill('0') << std::setw(3) << micros % 1000
					<< "\33[2;25H"
					<< Limiter->getElapsedMillisLast()
				#ifndef SDL_PLATFORM_WIN32
					<< "\33[3;6H"
					<< iGuest->fetchCPF() << "   "
				#endif
					<< std::endl;
			}
		} else { 
			iGuest->processFrame(); 
		}
	} else {
		if (kb.isPressed(KEY(ESCAPE))) {
			return SDL_APP_SUCCESS;
		}
	}

	BVS->renderPresent();

	bic::kb.updateCopy();
	bic::mb.updateCopy();

	return SDL_APP_CONTINUE;
}

void VM_Host::replaceGuest(const bool disable) {
	bic::kb.updateCopy();
	bic::mb.updateCopy();

	if (disable) {
		BVS->resetWindow();
		GameFileChecker::delCore();
	}

	if (initGameCore()) {
		Limiter->setLimiter(iGuest->fetchFramerate());
		BVS->changeTitle(HDM->getFileStem().c_str());
	} else {
		Limiter->setLimiter(30.0f);
		HDM->clearCachedFileData();
	}
}

void VM_Host::loadGameFile(const char* const filename, const bool alert) {
	if (alert) { BVS->raiseWindow(); }
	if (HDM->validateGameFile(filename)) {
		replaceGuest(false);
	}
}

void VM_Host::pauseSystem(const bool state) const noexcept {
	if (iGuest) { iGuest->isSystemStopped(state); }
}
