/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <SDL3/SDL.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "Assistants/BasicLogger.hpp"
#include "Assistants/BasicInput.hpp"
#include "Assistants/HomeDirManager.hpp"
#include "Assistants/BasicVideoSpec.hpp"
#include "Assistants/BasicAudioSpec.hpp"

#include "Cubechip.hpp"
#include "EmuHost.hpp"

#if _WIN32
	#pragma warning(push)
	#pragma warning(disable : 5039)
		#include <mbctype.h>
		#include <windows.h>
	#pragma warning(pop)
#endif

/*==================================================================*/

BasicLogger& blog{ *BasicLogger::create() };

/*==================================================================*/

SDL_AppResult SDL_AppInit(void **Host, int argc, char *argv[]) {
//             VS               OTHER
#if !defined(NDEBUG) || defined(DEBUG)
	{
		printf("SDL3 test dated: 23/03/25 (dd/mm/yy)\n");
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

#if _WIN32
	_setmbcp(CP_UTF8);
	setlocale(LC_CTYPE, ".UTF-8");
	SetConsoleOutputCP(CP_UTF8);
#endif

#ifdef SDL_PLATFORM_WIN32
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d");
	SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, "0");
#endif
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_SetHint(SDL_HINT_APP_NAME, AppName);
	SDL_SetAppMetadata(AppName, AppVer, nullptr);

	if (!EmuHost::assignComponents(
		HomeDirManager::create(nullptr, AppName),
		BasicVideoSpec::create(AppName),
		BasicAudioSpec::create()
	)) { return SDL_APP_FAILURE; }

	*Host = EmuHost::create(argc <= 1 ? "" : argv[1]);
	return SDL_APP_CONTINUE;
}

/*==================================================================*/

SDL_AppResult SDL_AppIterate(void *pHost) {
	auto& Host{ *static_cast<EmuHost*>(pHost) };
	const std::lock_guard lock{ Host.Mutex };

	Host.processFrame();

	return SDL_APP_CONTINUE;
}

/*==================================================================*/

SDL_AppResult SDL_AppEvent(void *pHost, SDL_Event *Event) {
	auto& Host{ *static_cast<EmuHost*>(pHost) };
	const std::lock_guard lock{ Host.Mutex };

	Host.processInterfaceEvent(Event);

	if (Host.isMainWindow(Event->window.windowID)) {
		static bool mainWindowPaused{};

		switch (Event->type) {
			case SDL_EVENT_QUIT:
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				return SDL_APP_SUCCESS;

			case SDL_EVENT_DROP_FILE:
				Host.loadGameFile(Event->drop.data);
				break;

			case SDL_EVENT_WINDOW_MINIMIZED:
				if (!mainWindowPaused) {
					Host.pauseSystem(mainWindowPaused = true);
				}
				break;

			case SDL_EVENT_WINDOW_RESTORED:
				if (mainWindowPaused) {
					Host.pauseSystem(mainWindowPaused = false);
				}
				break;

			case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
			case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
				Host.scaleInterface(AppFontData);
				break;
		}
	}

	return SDL_APP_CONTINUE;
}

/*==================================================================*/

void SDL_AppQuit(void *pHost, SDL_AppResult) {
	auto& Host{ *static_cast<EmuHost*>(pHost) };
	Host.quitApplication();
}
