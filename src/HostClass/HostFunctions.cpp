/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <iostream>
#include <iomanip>
#include <chrono>

#include "HomeDirManager.hpp"
#include "BasicVideoSpec.hpp"
#include "BasicAudioSpec.hpp"

#include "../Assistants/BasicLogger.hpp"
#include "../Assistants/BasicInput.hpp"
#include "../Assistants/FrameLimiter.hpp"

#include "Host.hpp"
#include "../GuestClass/Guest.hpp"
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
		if (eventLoopSDL(Guest, Frame)) {
			return EXIT_SUCCESS;
		}

		if (!Frame.check(doBench()
			? FrameLimiter::SPINLOCK
			: FrameLimiter::SLEEP
		)) { continue; }

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
				std::cout << "\33[1;1H\33[2J\33[?25l"
					<< "Cycle time:      ms |     μs";
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

				using namespace std::chrono;

				std::cout << "\33[2;1H" << std::dec << std::setfill(' ') << std::setprecision(6)
					<< "\nframes: " << Guest.getTotalFrames()     << "      "
					<< "\ncycles: " << Guest.getTotalCycles()     << "      "
					<< "\ncpf:    " << std::abs(Guest.fetchCPF()) << "      "
					<< (Frame.paced() ? "\n\n > keeping up pace." : "\n\n > cannot keep up!!")
					<< "\n\nelapsed since last: " << Frame.elapsed() << std::endl;

				auto start = high_resolution_clock::now();

				Guest.processFrame();

				auto end = high_resolution_clock::now();

				auto duration = end - start;
				auto ms = duration_cast<std::chrono::milliseconds>(duration);
				auto mu = duration_cast<std::chrono::microseconds>(duration - ms);

				std::cout
					<< "\33[1;13H" << std::setw(4) << ms.count()
					<< "\33[1;23H" << std::setw(3) << mu.count()
					<< "\33[1;1H";
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
