/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <iostream>
#include <iomanip>
#include <optional>
#include <thread>
#include <atomic>
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
		isReadyToEmulate(HDM.verifyFile(RomFile::validate, filename));
	}
}

bool VM_Host::isReadyToEmulate() const { return _isReadyToEmulate.load(std::memory_order_acquire); }
void VM_Host::isReadyToEmulate(const bool state) { _isReadyToEmulate.store(state, std::memory_order_release); }

void VM_Host::executeWorker(std::stop_token stopToken) {
	std::optional<VM_Guest> Guest{};
	FrameLimiter Frame;
	bool doBenchmarking{};

	if (isReadyToEmulate()) {
		Guest.emplace(HDM, BVS, BAS);

		if (Guest->setupMachine()) {
			Frame.setLimiter(Guest->fetchFramerate());
			BVS.changeTitle(HDM.file.c_str());
		} else {
			isReadyToEmulate(false);
			HDM.reset();
			return;
		}
	}

	while (!stopToken.stop_requested()) {
		if (!Frame.check(doBenchmarking
			? FrameLimiter::SPINLOCK
			: FrameLimiter::SLEEP
		)) { continue; }

		if (kb.isPressed(KEY(RIGHT))) {
			BAS.changeVolume(+15);
		}
		if (kb.isPressed(KEY(LEFT))) {
			BAS.changeVolume(-15);
		}

		if (isReadyToEmulate()) {
			if (kb.isPressed(KEY(RSHIFT))) {
				doBenchmarking = !doBenchmarking;
				std::cout << "\33[1;1H\33[2J\33[?25l"
					<< "Cycle time:      ms |     μs";
			}

			if (kb.isPressed(KEY(PAGEDOWN))) {
				BVS.changeFrameMultiplier(-1);
			}
			if (kb.isPressed(KEY(PAGEUP))) {
				BVS.changeFrameMultiplier(+1);
			}

			if (doBenchmarking) {
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
		}
	}
	printf("stop was requested and accepted!\n");
}

bool VM_Host::runHost() {
	SDL_Event Event;

	using namespace bic;

	prepareWorker();

	while (true) {
		while (SDL_PollEvent(&Event)) {
			switch (Event.type) {
				case SDL_EVENT_QUIT:
					disableWorker();
					return EXIT_SUCCESS;

				case SDL_EVENT_DROP_FILE:
					BVS.raiseWindow();
					if (HDM.verifyFile(RomFile::validate, Event.drop.data)) {
						isReadyToEmulate(true);
						prepareWorker();
					} else {
						blog.stdLogOut(std::string{ "File drop denied: " } + RomFile::error);
					}
					break;

				case SDL_EVENT_WINDOW_MINIMIZED:
					//if (Guest) { Guest->isSystemPaused(true); }
					break;

				case SDL_EVENT_WINDOW_RESTORED:
					//if (Guest) { Guest->isSystemPaused(false); }
					break;
			}
		}

		if (isReadyToEmulate()) {
			if (kb.isPressed(KEY(ESCAPE))) {
				printf("worker ESC key detected\n");
				isReadyToEmulate(false);
				BVS.resetWindow();
				prepareWorker();
				continue;
			}
			if (kb.isPressed(KEY(BACKSPACE))) {
				prepareWorker();
				continue;
			}
		} else {
			if (kb.isPressed(KEY(ESCAPE))) {
				printf("attempting exit!\n");
				return EXIT_SUCCESS;
			}
		}

		BVS.renderPresent();

		kb.updateCopy();
		mb.updateCopy();
	}
}

void VM_Host::disableWorker() {
	printf("disableWorker() called\n");
	if (workerGuest.joinable()) {
		workerGuest.request_stop();
		printf("worker STOP requested\n");
		workerGuest.join();
		printf("worker JOIN completed\n");
	}
}

void VM_Host::prepareWorker() {

	bic::kb.updateCopy();
	bic::mb.updateCopy();

	disableWorker();

	workerGuest = std::jthread([this](std::stop_token stopToken) {
		executeWorker(stopToken);
	});
}
