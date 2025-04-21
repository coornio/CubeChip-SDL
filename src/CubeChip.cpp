/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "Assistants/BasicLogger.hpp"
#include "Assistants/BasicInput.hpp"
#include "Assistants/ThreadAffinity.hpp"

#include "Cubechip.hpp"
#include "EmuHost.hpp"

#ifdef _WIN32
	#pragma warning(push)
	#pragma warning(disable : 5039)
		#include <mbctype.h>
	#pragma warning(pop)
	#define NOMINMAX
	#include <windows.h>
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

#ifdef _WIN32
	_setmbcp(CP_UTF8);
	setlocale(LC_CTYPE, ".UTF-8");
	SetConsoleOutputCP(CP_UTF8);
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif

	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_SetHint(SDL_HINT_APP_NAME, AppName);
	SDL_SetAppMetadata(AppName, AppVer, nullptr);

	// XXX - forward cxx flags to initApplication
	if (!EmuHost::initApplication())
		{ return SDL_APP_FAILURE; }

	*Host = EmuHost::create(argc <= 1 ? "" : argv[1]);

	thread_affinity::set_affinity(0b11ull);

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

SDL_AppResult SDL_AppEvent(void *pHost, SDL_Event *event) {
	auto& Host{ *static_cast<EmuHost*>(pHost) };
	const std::lock_guard lock{ Host.Mutex };

	return Host.processEvents(event);
}

/*==================================================================*/

void SDL_AppQuit(void *pHost, SDL_AppResult) {
	auto& Host{ *static_cast<EmuHost*>(pHost) };
	Host.quitApplication();
}
