/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Assistants/FrameLimiter.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <optional>
#include <memory>
#include <mutex>

#include "HostClass/HomeDirManager.hpp"
#include "HostClass/BasicVideoSpec.hpp"
#include "HostClass/BasicAudioSpec.hpp"
#include "Assistants/BasicInput.hpp"

#include "HostClass/Host.hpp"

struct {
	std::optional<HomeDirManager> HDM;
	std::optional<BasicVideoSpec> BVS;
	std::optional<BasicAudioSpec> BAS;
	FrameLimiter Limiter;
	std::unique_ptr<VM_Host> Host;
	std::mutex Mut;
} global;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char* argv[]) {

	SDL_InitSubSystem(SDL_INIT_EVENTS);

//             VS               OTHER
#if !defined(NDEBUG) || defined(DEBUG)
	{
		printf("SDL3 source dated: 18/8/2024\n");
		const auto compiled{ SDL_VERSION };  /* hardcoded number from SDL headers */
		const auto linked{ SDL_GetVersion() };  /* reported by linked SDL library */

		printf("Compiled with SDL version: %d.%d.%d\n",
			SDL_VERSIONNUM_MAJOR(compiled),
			SDL_VERSIONNUM_MINOR(compiled),
			SDL_VERSIONNUM_MICRO(compiled)
		);
		printf("Linked with SDL version: %d.%d.%d\n",
			SDL_VERSIONNUM_MAJOR(linked),
			SDL_VERSIONNUM_MINOR(linked),
			SDL_VERSIONNUM_MICRO(linked)
		);
	}
#endif

#ifdef SDL_PLATFORM_WIN32
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d");
	SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, "0");
#endif
	SDL_SetHint(SDL_HINT_APP_NAME, "CubeChip");

	global.Mut.lock();

	try {
		global.HDM.emplace("CubeChip_SDL");
		global.BVS.emplace();
		global.BAS.emplace();
	} catch (...) { return SDL_APP_FAILURE; }

	global.Host = std::make_unique<VM_Host>(
		argc <= 1 ? nullptr : argv[1],
		*global.HDM, *global.BVS, *global.BAS
	);
	global.Host->prepareGuest(global.Limiter);

	global.Mut.unlock();

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	global.Mut.lock();

	if (!global.Host->runFrame(global.Limiter)) {
		global.Mut.unlock();
		return SDL_APP_SUCCESS;
	}

	global.Host->BVS.renderPresent();

	global.Mut.unlock();

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, const SDL_Event *event) {
	global.Mut.lock();

	if (!global.Host->handleEventSDL(global.Limiter, event)) {
		global.Mut.unlock();
		return SDL_APP_SUCCESS;
	}

	global.Mut.unlock();
	
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate) {}
