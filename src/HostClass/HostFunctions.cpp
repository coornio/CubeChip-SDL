/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <sstream>
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
	if (filename) {
		HDM.verifyFile(GameFileChecker::validate, filename);
	}
}

bool VM_Host::doBench() const noexcept { return _doBench; }
void VM_Host::doBench(const bool state) noexcept { _doBench = state; }


bool VM_Host::runHost() {
	FrameLimiter Frame;
	VM_Guest     Guest;

	using namespace bic;

	prepareGuest(Guest, Frame);

	while (true) {
		if (!Frame.checkTime()) { continue; }

		if (eventLoopSDL(Guest, Frame)) {
			return EXIT_SUCCESS;
		}

		if (kb.isPressed(KEY(RIGHT))) {
			BAS.changeVolume(+15);
		}
		if (kb.isPressed(KEY(LEFT))) {
			BAS.changeVolume(-15);
		}

		if (GameFileChecker::hasCore()) {
			if (kb.isPressed(KEY(ESCAPE))) {
				BVS.resetWindow();
				GameFileChecker::delCore();
				prepareGuest(Guest, Frame);
				continue;
			}
			if (kb.isPressed(KEY(BACKSPACE))) {
				prepareGuest(Guest, Frame);
				continue;
			}
			if (kb.isPressed(KEY(RSHIFT))) {
				doBench(!doBench());
				if (!doBench()) {
					BVS.changeTitle(HDM.file.c_str());
				}
			}

			if (kb.isPressed(KEY(PAGEDOWN))) {
				BVS.changeFrameMultiplier(-1);
			}
			if (kb.isPressed(KEY(PAGEUP))) {
				BVS.changeFrameMultiplier(+1);
			}

			if (doBench()) {
				if (kb.isPressed(KEY(UP))) {
					Guest.changeCPF(+50'000);
				}
				if (kb.isPressed(KEY(DOWN))) {
					Guest.changeCPF(-50'000);
				}

				Guest.processFrame();

				{
					const auto micros{ Frame.getElapsedMicrosSince() };
					std::ostringstream out;

					out << Guest.fetchCPF() << " :: " << (micros / 1000) << ".";
					out << std::setfill('0') << std::setw(3) << (micros % 1000) << " :: ";
					out << std::left << Frame.getElapsedMillisLast();
					BVS.changeTitle(out.str());
				}
			} else { Guest.processFrame(); }
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

void VM_Host::prepareGuest(VM_Guest& Guest, FrameLimiter& Frame) {
	bic::kb.updateCopy();
	bic::mb.updateCopy();

	if (GameFileChecker::hasCore()) {
		Guest.initGameCore(HDM, BVS, BAS);
		Frame.setLimiter(Guest.fetchFramerate());
		BVS.changeTitle(HDM.file.c_str());
	} else {
		Frame.setLimiter(30.0f);
		HDM.reset();
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
				HDM.verifyFile(GameFileChecker::validate, Event.drop.data);
				if (GameFileChecker::hasCore()) {
					prepareGuest(Guest, Frame);
				} else {
					blog.stdLogOut(std::string{ "File drop denied: " } + GameFileChecker::getError());
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
