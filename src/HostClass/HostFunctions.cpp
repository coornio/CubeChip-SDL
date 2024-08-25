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

VM_Host::~VM_Host() = default;
VM_Host::VM_Host(
	const char* const filename,
	HomeDirManager&   ref_HDM,
	BasicVideoSpec&   ref_BVS,
	BasicAudioSpec&   ref_BAS
)
	: HDM{ ref_HDM }
	, BVS{ ref_BVS }
	, BAS{ ref_BAS }
{
	HDM.setValidator(GameFileChecker::validate);
	HDM.validateGameFile(filename);
}

bool VM_Host::doBench() const noexcept { return _doBench; }
void VM_Host::doBench(const bool state) noexcept { _doBench = state; }

bool VM_Host::initGameCore() {
	iGuest = std::move(GameFileChecker::initializeCore(HDM, BVS, BAS));
	return iGuest ? true : false;
}

bool VM_Host::runHost() {
	FrameLimiter Frame;

	using namespace bic;

	prepareGuest(Frame);

	while (true) {
		if (!Frame.checkTime()) [[likely]] { continue; }

		if (eventLoopSDL(Frame)) {
			return EXIT_SUCCESS;
		}

		if (kb.isPressed(KEY(RIGHT))) {
			BAS.changeVolume(+15);
		}
		if (kb.isPressed(KEY(LEFT))) {
			BAS.changeVolume(-15);
		}

		if (iGuest) {
			if (kb.isPressed(KEY(ESCAPE))) {
				BVS.resetWindow();
				GameFileChecker::delCore();
				prepareGuest(Frame);
				continue;
			}
			if (kb.isPressed(KEY(BACKSPACE))) {
				if (HDM.validateGameFile(HDM.getFilePath().c_str())) {
					prepareGuest(Frame);
				}
				continue;
			}
			if (kb.isPressed(KEY(RSHIFT))) {
				if (doBench()) {
					doBench(false);
					BVS.changeTitle(HDM.getFileStem().c_str());
					std::cout << "\33[1;1H\33[3J" << std::endl;
				} else {
					doBench(true);
					BVS.changeTitle(std::to_string(iGuest->fetchCPF()));
					std::cout << "\33[1;1H\33[2J"
						<< "Cycle time:    .    ms"
						<< "\nTime since last frame: "
						<< "\nCPF: ";
				}
			}

			if (kb.isPressed(KEY(PAGEDOWN))){
				BVS.changeFrameMultiplier(-1);
			}
			if (kb.isPressed(KEY(PAGEUP))) {
				BVS.changeFrameMultiplier(+1);
			}

			if (doBench()) [[likely]] {
				if (kb.isPressed(KEY(UP))) {
					BVS.changeTitle(std::to_string(iGuest->changeCPF(+50'000)));
				}
				if (kb.isPressed(KEY(DOWN))){
					BVS.changeTitle(std::to_string(iGuest->changeCPF(-50'000)));
				}

				iGuest->processFrame();

				if (Frame.getValidFrameCounter() & 0x1) {
					const auto micros{ Frame.getElapsedMicrosSince() };
					std::cout
						<< "\33[1;12H"
						<< std::setfill(' ') << std::setw(4) << micros / 1000
						<< "\33[1C"
						<< std::setfill('0') << std::setw(3) << micros % 1000
						<< "\33[2;25H"
						<< Frame.getElapsedMillisLast()
						<< "\33[3;6H"
						<< iGuest->fetchCPF() << "   "
						<< std::endl;
				}
			} else { iGuest->processFrame(); }
		} else {
			if (kb.isPressed(KEY(ESCAPE))) {
				return EXIT_SUCCESS;
			}
		}

		BVS.renderPresent();

		kb.updateCopy();
		mb.updateCopy();
	}
}

void VM_Host::prepareGuest(FrameLimiter& Frame) {
	bic::kb.updateCopy();
	bic::mb.updateCopy();

	if (initGameCore()) {
		Frame.setLimiter(iGuest->fetchFramerate());
		BVS.changeTitle(HDM.getFileStem().c_str());
	} else {
		Frame.setLimiter(30.0f);
		HDM.clearCachedFileData();
	}
}

bool VM_Host::eventLoopSDL(FrameLimiter& Frame) {
	SDL_Event Event;

	while (SDL_PollEvent(&Event)) {
		switch (Event.type) {
			case SDL_EVENT_QUIT:
				return true;

			case SDL_EVENT_DROP_FILE:
				BVS.raiseWindow();
				if (HDM.validateGameFile(Event.drop.data)) {
					prepareGuest(Frame);
				}
				break;

			case SDL_EVENT_WINDOW_MINIMIZED:
				if (iGuest) { iGuest->isSystemStopped(true); }
				break;

			case SDL_EVENT_WINDOW_RESTORED:
				if (iGuest) { iGuest->isSystemStopped(false); }
				break;
		}
	}
	return false;
}
