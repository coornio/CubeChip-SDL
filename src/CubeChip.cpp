/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <optional>

#include "HostClass/HomeDirManager.hpp"
#include "HostClass/BasicVideoSpec.hpp"
#include "HostClass/BasicAudioSpec.hpp"

#include "HostClass/Host.hpp"

int SDL_main(int argc, char* argv[]) {

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
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1"); // until the UI is independent
	SDL_SetHint(SDL_HINT_APP_NAME, "CubeChip");

	alignas(64) std::optional<HomeDirManager> HDM;
	alignas(64) std::optional<BasicVideoSpec> BVS;
	alignas(64) std::optional<BasicAudioSpec> BAS;

	try {
		HDM.emplace("CubeChip_SDL");
		BVS.emplace();
		BAS.emplace();
	} catch (...) { return EXIT_FAILURE; }

	VM_Host Host(
		argc <= 1 ? nullptr : argv[1],
		*HDM, *BVS, *BAS
	);

	return Host.runHost();
}
