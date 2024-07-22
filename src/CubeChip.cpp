/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <memory>

#include "HostClass/Host.hpp"

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

	VM_Host Host(argc <= 1 ? nullptr : argv[1]);
	if (Host.initFailure()) { return EXIT_FAILURE; }

	while (true) {
		return Host.runHost();
	}
}
