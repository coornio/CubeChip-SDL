/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <SDL3/SDL_main.h>
#include "Includes.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>

int32_t SDL_main(int32_t argc, char* argv[]) {

	atexit(SDL_Quit);

	#ifdef _DEBUG
	{
		SDL_Version compiled{}; SDL_VERSION(&compiled);
		SDL_Version linked{};   SDL_GetVersion(&linked);

		printf("Compiled with SDL version %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch);
		printf("Linked with SDL version %d.%d.%d\n", linked.major, linked.minor, linked.patch);
	}
	#endif

	SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, "0");
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d");
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0"); // until the UI is independent
	SDL_SetHint(SDL_HINT_APP_NAME, "CubeChip");

	std::unique_ptr<HomeDirManager> HDM;
	std::unique_ptr<BasicVideoSpec> BVS;
	std::unique_ptr<BasicAudioSpec> BAS;
	std::unique_ptr<VM_Guest>       Guest;

	try {
		HDM = std::make_unique<HomeDirManager>("CubeChip_SDL");
		BVS = std::make_unique<BasicVideoSpec>(1040, 528);
		BAS = std::make_unique<BasicAudioSpec>(48'000);
	} catch (...) { return EXIT_FAILURE; }

	VM_Host      Host(HDM.get(), BVS.get(), BAS.get());
	FrameLimiter Frame(60.0, true, true);
	SDL_Event    Event;

	Host.isReady(HDM->verifyFile(argc > 1 ? argv[1] : nullptr));

reset_all:
	Guest = nullptr;
	kb.updateCopy();
	mb.updateCopy();

	if (Host.isReady()) {
		Guest = std::make_unique<VM_Guest>(HDM.get(), BVS.get(), BAS.get());

		if (Guest->setupMachine()) {
			Frame.setLimiter(Guest->fetchFramerate());
			BVS->changeTitle(HDM->file.c_str());
		} else {
			Frame.setLimiter(30.0);
			Host.isReady(false);
			HDM->reset();
		}
	}

	while (true) {
		while (SDL_PollEvent(&Event)) {
			switch (Event.type) {
				case SDL_EVENT_QUIT:
					return EXIT_SUCCESS;

				case SDL_EVENT_DROP_FILE:
					if (HDM->verifyFile(Event.drop.data)) {
						blog.stdLogOut("File drop accepted: "s + Event.drop.data);
						Host.isReady(true);
						goto reset_all;
					} else {
						blog.stdLogOut("File drop denied: "s + Event.drop.data);
						break;
					}

				case SDL_EVENT_WINDOW_MINIMIZED:
					Guest->isSystemPaused(true);
					break;

				case SDL_EVENT_WINDOW_RESTORED:
					Guest->isSystemPaused(false);
					break;
			}
		}

		if (!Frame.check(Host.doBench()
			? FrameLimiter::SPINLOCK
			: FrameLimiter::SLEEP
		)) continue;

		if (Host.isReady()) {
			if (kb.isPressed(KEY(ESCAPE))) {
				Host.isReady(false);
				BVS->changeTitle();
				BVS->createTexture();
				BVS->renderPresent();
				goto reset_all;
			}
			if (kb.isPressed(KEY(BACKSPACE))) {
				goto reset_all;
			}
			if (kb.isPressed(KEY(RSHIFT))) {
				Host.doBench(!Host.doBench());
				std::cout << "\33[1;1H\33[2J\33[?25l"
					<< "Cycle time:      ms |     μs";
			}
			if (kb.isPressed(KEY(UP))) {
				BAS->changeVolume(+15);
			}
			if (kb.isPressed(KEY(DOWN))) {
				BAS->changeVolume(-15);
			}

			if (Host.doBench()) {
				using namespace std::chrono;

				std::cout << "\33[2;1H" << std::dec << std::setfill(' ') << std::setprecision(6)
					<< "\ncycle: " << Host.cycles++
					<< "\nipf:   " << std::abs(Guest->fetchIPF())
					<< (Frame.paced() ? "\n\n > keeping up pace."s : "\n\n > cannot keep up!!"s)
					<< "\n\nelapsed since last: " << Frame.elapsed() << std::endl;

				auto start = high_resolution_clock::now();
				if (!Guest->isSystemPaused()) {
					Guest->cycle();
				}
				auto end = high_resolution_clock::now();
				auto duration = end - start;
				auto ms = duration_cast<std::chrono::milliseconds>(duration);
				auto mu = duration_cast<std::chrono::microseconds>(duration - ms);

				std::cout
					<< "\33[1;13H" << std::setw(4) << ms.count()
					<< "\33[1;23H" << std::setw(3) << mu.count();
			}
			else if (!Guest->isSystemPaused()) {
				Guest->cycle();
			}
		} else {
			if (kb.isPressed(KEY(ESCAPE))) {
				return EXIT_SUCCESS;
			}
		}

		BVS->renderPresent();

		kb.updateCopy();
		mb.updateCopy();
	}
}
