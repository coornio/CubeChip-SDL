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


bool VM_Host::runHost() {
	FrameLimiter Frame;
	VM_Guest     Guest;

	using namespace bic;

	prepareGuest(Guest, Frame);

	while (true) {
		if (!Frame.checkTime()) [[likely]] { continue; }

		if (eventLoopSDL(Guest, Frame)) [[unlikely]] {
			return EXIT_SUCCESS;
		}

		if (kb.isPressed(KEY(RIGHT))) [[unlikely]] {
			BAS.changeVolume(+15);
		}
		if (kb.isPressed(KEY(LEFT))) [[unlikely]] {
			BAS.changeVolume(-15);
		}

		if (GameFileChecker::hasCore()) {
			if (kb.isPressed(KEY(ESCAPE))) [[unlikely]] {
				BVS.resetWindow();
				GameFileChecker::delCore();
				prepareGuest(Guest, Frame);
				continue;
			}
			if (kb.isPressed(KEY(BACKSPACE))) [[unlikely]] {
				if (HDM.validateGameFile(HDM.getFilePath().c_str())) {
					prepareGuest(Guest, Frame);
				}
				continue;
			}
			if (kb.isPressed(KEY(RSHIFT))) [[unlikely]] {
				if (doBench()) {
					doBench(false);
					BVS.changeTitle(HDM.getFileStem().c_str());
					std::cout << "\33[1;1H\33[3J" << std::endl;
				} else {
					doBench(true);
					BVS.changeTitle(std::to_string(Guest.fetchCPF()));
					std::cout << "\33[1;1H\33[2J"
						<< "Cycle time:    .    ms"
						<< "\nTime since last frame: ";
				}
			}

			if (kb.isPressed(KEY(PAGEDOWN))) [[unlikely]] {
				BVS.changeFrameMultiplier(-1);
			}
			if (kb.isPressed(KEY(PAGEUP))) [[unlikely]] {
				BVS.changeFrameMultiplier(+1);
			}

			if (doBench()) [[likely]] {
				if (kb.isPressed(KEY(UP))) [[unlikely]] {
					BVS.changeTitle(std::to_string(Guest.changeCPF(+50'000)));
				}
				if (kb.isPressed(KEY(DOWN))) [[unlikely]] {
					BVS.changeTitle(std::to_string(Guest.changeCPF(-50'000)));
				}

				Guest.processFrame();

				if (Frame.getValidFrameCounter() & 0x1) {
					const auto micros{ Frame.getElapsedMicrosSince() };
					std::cout
						<< "\33[1;12H"
						<< std::setfill(' ') << std::setw(4) << micros / 1000
						<< "\33[1C"
						<< std::setfill('0') << std::setw(3) << micros % 1000
						<< "\33[2;25H"
						<< Frame.getElapsedMillisLast()
						<< std::endl;
				}
			} else { Guest.processFrame(); }
		} else {
			if (kb.isPressed(KEY(ESCAPE))) [[unlikely]] {
				return EXIT_SUCCESS;
			}
		}

		BVS.renderPresent();

		kb.updateCopy();
		mb.updateCopy();
	}
}

void VM_Host::prepareGuest(VM_Guest& Guest, FrameLimiter& Frame) {
	bic::kb.updateCopy();
	bic::mb.updateCopy();

	if (Guest.initGameCore(HDM, BVS, BAS)) {
		Frame.setLimiter(Guest.fetchFramerate());
		BVS.changeTitle(HDM.getFileStem().c_str());
	} else {
		Frame.setLimiter(30.0f);
		HDM.clearCachedFileData();
	}
}

bool VM_Host::eventLoopSDL(VM_Guest& Guest, FrameLimiter& Frame) {
	SDL_Event Event;

	while (SDL_PollEvent(&Event)) {
		switch (Event.type) {
			case SDL_EVENT_QUIT:
				return true;

			case SDL_EVENT_DROP_FILE:
				BVS.raiseWindow();
				if (HDM.validateGameFile(Event.drop.data)) {
					prepareGuest(Guest, Frame);
				}
				break;

			case SDL_EVENT_WINDOW_MINIMIZED:
				Guest.isSystemStopped(true);
				break;

			case SDL_EVENT_WINDOW_RESTORED:
				Guest.isSystemStopped(false);
				break;
		}
	}
	return false;
}
