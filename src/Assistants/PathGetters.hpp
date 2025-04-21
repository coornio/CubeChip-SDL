/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL3/SDL_filesystem.h>

inline auto getHomePath(
	const char* const org = nullptr,
	const char* const app = nullptr
) noexcept {
	static const char* homePath{ SDL_GetPrefPath(org, app) };
	return homePath;
}

inline auto getBasePath() noexcept {
	static const char* homePath{ SDL_GetBasePath() };
	return homePath;
}
