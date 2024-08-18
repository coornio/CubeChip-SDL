/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_version.h>
#include <optional>

#include "HostClass/HomeDirManager.hpp"
#include "HostClass/BasicVideoSpec.hpp"
#include "HostClass/BasicAudioSpec.hpp"

#include "HostClass/Host.hpp"

int main(int argc, char* argv[]) {

	atexit(SDL_Quit);

	{
		const int compiled_version = SDL_VERSION;
		const int linked_version = SDL_GetVersion();
		printf("Compiled with SDL version %d.%d.%d\n", SDL_VERSIONNUM_MAJOR(compiled_version), SDL_VERSIONNUM_MINOR(compiled_version), SDL_VERSIONNUM_MICRO(compiled_version));
		printf("Linked with SDL version %d.%d.%d\n", SDL_VERSIONNUM_MAJOR(linked_version), SDL_VERSIONNUM_MINOR(linked_version), SDL_VERSIONNUM_MICRO(linked_version));
	}

	SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, "0");
  #ifdef SDL_PLATFORM_WIN32
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d");
	#endif
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0"); // until the UI is independent
	SDL_SetHint(SDL_HINT_APP_NAME, "CubeChip");

	std::optional<HomeDirManager> HDM;
	std::optional<BasicVideoSpec> BVS;
	std::optional<BasicAudioSpec> BAS;

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
