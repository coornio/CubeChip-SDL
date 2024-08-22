/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <SDL3/SDL.h>

#include "BasicHome.hpp"
#include "PathExceptionClass.hpp"

bool BasicHome::showErrorBox(
	std::string_view message,
	std::string_view title
) {
	return SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title.data(),
		message.data(), nullptr
	);
}

BasicHome::BasicHome(const std::string_view homeName) {
	auto* path{ SDL_GetPrefPath(nullptr, homeName.data()) };

	if (!path) {
		throw PathException("Failed to get platform home directory!", "");
	}

	mHomeDirectory.assign(path);
	SDL_free(path);

	namespace fs = std::filesystem;

	fs::create_directories(mHomeDirectory);
	if (!fs::exists(mHomeDirectory) || !fs::is_directory(mHomeDirectory)) {
		throw PathException("Cannot create home directory: ", mHomeDirectory);
	}
}
