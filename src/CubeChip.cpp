/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <SDL3/SDL.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#if 0
#include <windows.h>
#include <shellapi.h>
#endif

#include "HostClass/HomeDirManager.hpp"
#include "HostClass/BasicVideoSpec.hpp"
#include "HostClass/BasicAudioSpec.hpp"
#include "Assistants/BasicInput.hpp"

#include "HostClass/Host.hpp"

SDL_AppResult SDL_AppInit(void **Host, int argc, char* argv[]) {

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

#if 0
	setlocale(LC_CTYPE, "");
	LPWSTR* wargv{}; int wargc{};
	wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
	if (!wargv) { return SDL_APP_FAILURE; }

	*Host = VM_Host::initialize(wargc <= 1 ? L"" : wargv[1]);
	LocalFree(wargv);
#else
	*Host = VM_Host::initialize(argc <= 1 ? "" : argv[1]);
#endif

	if (Host) { return SDL_APP_CONTINUE; }
	else      { return SDL_APP_FAILURE;  }
}

SDL_AppResult SDL_AppIterate(void* pHost) {
	auto& Host{ *static_cast<VM_Host*>(pHost) };

	Host.Mutex.lock();
	switch (Host.runFrame()) {
		case SDL_APP_SUCCESS:
			Host.Mutex.unlock();
			return SDL_APP_SUCCESS;

		case SDL_APP_CONTINUE:
			Host.Mutex.unlock();
			return SDL_APP_CONTINUE;

		case SDL_APP_FAILURE:
			Host.Mutex.unlock();
			return SDL_APP_FAILURE;
	}
}

SDL_AppResult SDL_AppEvent(void* pHost, const SDL_Event* Event) {
	auto& Host{ *static_cast<VM_Host*>(pHost) };

	switch (Event->type) {
		case SDL_EVENT_QUIT:
			return SDL_APP_SUCCESS;

		case SDL_EVENT_DROP_FILE:
			Host.Mutex.lock();
			Host.loadGameFile(Event->drop.data, true);
			Host.Mutex.unlock();
			break;

		case SDL_EVENT_WINDOW_MINIMIZED:
			Host.Mutex.lock();
			Host.pauseSystem(true);
			Host.Mutex.unlock();
			break;

		case SDL_EVENT_WINDOW_RESTORED:
			Host.Mutex.lock();
			Host.pauseSystem(false);
			Host.Mutex.unlock();
			break;
	}
	
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void*) {}
