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

#include "Assistants/BasicLogger.hpp"
#include "Assistants/BasicInput.hpp"
#include "Assistants/HomeDirManager.hpp"
#include "Assistants/BasicVideoSpec.hpp"
#include "Assistants/BasicAudioSpec.hpp"

#include "EmuHost.hpp"

/*==================================================================*/

BasicLogger&         blog{   *BasicLogger::create() };
BasicKeyboard& binput::kb{ *BasicKeyboard::create() };
BasicMouse&    binput::mb{    *BasicMouse::create() };

/*==================================================================*/

SDL_AppResult SDL_AppInit(void **Host, int argc, char* argv[]) {

//             VS               OTHER
#if !defined(NDEBUG) || defined(DEBUG)
	{
		printf("SDL3 test dated: 20/10/24 (dd/mm/yy)\n");
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

	if (EmuHost::assignComponents(
		HomeDirManager::create(nullptr, "CubeChip"),
		BasicVideoSpec::create(),
		BasicAudioSpec::create()
	)) { return SDL_APP_FAILURE; }

#if 0
	setlocale(LC_CTYPE, "");
	LPWSTR* wargv{}; int wargc{};
	wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
	if (!wargv) { return SDL_APP_FAILURE; }

	*Host = &EmuHost::create(wargc <= 1 ? L"" : wargv[1]);
	LocalFree(wargv);
#else
	*Host = EmuHost::create(argc <= 1 ? "" : argv[1]);
#endif
	return SDL_APP_CONTINUE;
}

/*==================================================================*/

SDL_AppResult SDL_AppIterate(void* pHost) {
	auto& Host{ *static_cast<EmuHost*>(pHost) };
	const std::lock_guard lock{ Host.Mutex };

	Host.processFrame();

	return SDL_APP_CONTINUE;
}

/*==================================================================*/

SDL_AppResult SDL_AppEvent(void* pHost, SDL_Event* Event) {
	auto& Host{ *static_cast<EmuHost*>(pHost) };
	const std::lock_guard lock{ Host.Mutex };

	ImGui_ImplSDL3_ProcessEvent(Event);

	switch (Event->type) {
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			return Host.terminationRequested(Event->window.windowID)
				? SDL_APP_SUCCESS
				: SDL_APP_CONTINUE;

		case SDL_EVENT_DROP_FILE:
			Host.loadGameFile(Event->drop.data);
			break;

		case SDL_EVENT_WINDOW_MINIMIZED:
			Host.pauseSystem(true);
			break;

		case SDL_EVENT_WINDOW_RESTORED:
			Host.pauseSystem(false);
			break;
	}

	return SDL_APP_CONTINUE;
}

/*==================================================================*/

void SDL_AppQuit(void*, SDL_AppResult) {}
