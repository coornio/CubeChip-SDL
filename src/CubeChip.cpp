/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <SDL3/SDL_main.h>
#include "Includes.hpp"

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

	// low latency mode is unreliable, best go with OS messaging queue
	SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, "0");

	// direct3d requires less memory, and we don't do anything fancy
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d");

	std::unique_ptr<HomeDirManager> HDM;
	std::unique_ptr<BasicVideoSpec> BVS;
	std::unique_ptr<BasicAudioSpec> BAS;
	std::unique_ptr<VM_Guest>       Guest;

	try {
		HDM = std::make_unique<HomeDirManager>("CubeChip_SDL");
		BVS = std::make_unique<BasicVideoSpec>(1000, 500);
		BAS = std::make_unique<BasicAudioSpec>(48'000);
	} catch (...) { return EXIT_FAILURE; }

	VM_Host      Host(HDM.get(), BVS.get(), BAS.get());
	FrameLimiter Frame(60.0, true, true);
	SDL_Event    Event;

	if (HDM->verifyFile(argc > 1 ? argv[1] : nullptr)) {
		Host.isReady(true);
	}

reset_all:
	kb.updateCopy();
	mb.updateCopy();

	if (Host.isReady()) {
		Guest = nullptr; // destroy old instance, and create a new one
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

				case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
					// do some maths later for ensuring saner scaling
					break;

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

		if (kb.isPressed(KEY(ESCAPE))) {
			if (Host.isReady()) {
				Host.isReady(false);
				BVS->changeTitle();
				BVS->createTexture(1, 1);
				BVS->renderPresent();
				goto reset_all;
			} else {
				return EXIT_SUCCESS;
			}
		}
		if (kb.isPressed(KEY(RSHIFT))) {
			Host.doBench(!Host.doBench());
		}
		if (kb.isPressed(KEY(UP))) {
			BAS->changeVolume(+15);
		}
		if (kb.isPressed(KEY(DOWN))) {
			BAS->changeVolume(-15);
		}

		if (Host.isReady()) {
			if (kb.isPressed(KEY(BACKSPACE))) {
				goto reset_all;
			}

			if (!Guest->isSystemPaused()) {
				Guest->cycle();
			}
		}

		BVS->renderPresent();
		kb.updateCopy();
		mb.updateCopy();
	}
}
