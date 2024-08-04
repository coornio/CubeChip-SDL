/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

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
#include "../GuestClass/RomCheck.hpp"

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
		isReady(HDM.verifyFile(RomFile::validate, filename));
	}
}

bool VM_Host::isReady() const noexcept { return _isReady; }
bool VM_Host::doBench() const noexcept { return _doBench; }

void VM_Host::isReady(const bool state) noexcept { _isReady = state; }
void VM_Host::doBench(const bool state) noexcept { _doBench = state; }


bool VM_Host::runHost() {
	FrameLimiter Frame;

	std::optional<VM_Guest> Guest;

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

		if (isReady()) {
			if (kb.isPressed(KEY(ESCAPE))) {
				isReady(false);
				doBench(false);
				BVS.resetWindow();
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
				using namespace std::chrono;

				std::cout << "\33[2;1H" << std::dec << std::setfill(' ') << std::setprecision(6)
					<< "\33[K\nframes: " << Guest->getTotalFrames()
					<< "\33[K\ncycles: " << Guest->getTotalCycles()
					<< "\33[K\ncpf:    " << std::abs(Guest->fetchCPF())
					<< (Frame.paced() ? "\n\n > keeping up pace." : "\n\n > cannot keep up!!")
					<< "\n\nelapsed since last: " << Frame.elapsed() << std::endl;

				auto start = high_resolution_clock::now();

				Guest->processFrame();

				auto end = high_resolution_clock::now();

				auto duration = end - start;
				auto ms = duration_cast<std::chrono::milliseconds>(duration);
				auto mu = duration_cast<std::chrono::microseconds>(duration - ms);

				std::cout
					<< "\33[1;13H" << std::setw(4) << ms.count()
					<< "\33[1;23H" << std::setw(3) << mu.count()
					<< "\33[1;1H";
			} else { Guest->processFrame(); }
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

void VM_Host::prepareGuest(std::optional<VM_Guest>& Guest, FrameLimiter& Frame) {
	Guest.reset();
	bic::kb.updateCopy();
	bic::mb.updateCopy();

	if (isReady()) {
		Guest.emplace(HDM, BVS, BAS);

		if (Guest->setupMachine()) {
			Frame.setLimiter(Guest->fetchFramerate());
			BVS.changeTitle(HDM.file.c_str());
		} else {
			Frame.setLimiter(30.0f);
			isReady(false);
			HDM.reset();
		}
	}
}

bool VM_Host::eventLoopSDL(std::optional<VM_Guest>& Guest, FrameLimiter& Frame) {
	SDL_Event Event;

	while (SDL_PollEvent(&Event)) {
		switch (Event.type) {
			case SDL_EVENT_QUIT:
				return true;

			case SDL_EVENT_DROP_FILE:
				BVS.raiseWindow();
				if (HDM.verifyFile(RomFile::validate, Event.drop.data)) {
					isReady(true);
					doBench(false);
					prepareGuest(Guest, Frame);
				} else {
					blog.stdLogOut(std::string{ "File drop denied: " } + RomFile::error);
				}
				break;

			case SDL_EVENT_WINDOW_MINIMIZED:
				if (Guest) { Guest->isSystemStopped(true); }
				break;

			case SDL_EVENT_WINDOW_RESTORED:
				if (Guest) { Guest->isSystemStopped(false); }
				break;
		}
	}
	return false;
}
