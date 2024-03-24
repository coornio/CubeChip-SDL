/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicHome.hpp"
#include "PathExceptionClass.hpp"
#include <SDL.h>

/*------------------------------------------------------------------*/
/*  class  VM_Host::FileInfo                                         */
/*------------------------------------------------------------------*/

bool BasicHome::showErrorBox(
    std::string_view message,
    std::string_view title
) {
    return SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR, title.data(),
        message.data(), nullptr
    );
}

BasicHome::BasicHome(const char* path) {
    char* platformHome{};
    if (!(platformHome = SDL_GetPrefPath(nullptr, path))) {
        throw PathException("Failed to get platform home directory!", "");
    }

    homeDir = platformHome;
    SDL_free(reinterpret_cast<void*>(platformHome));

    std::filesystem::create_directories(homeDir);
    if (!std::filesystem::exists(homeDir)) {
        throw PathException("Cannot create home directory: ", homeDir);
    }
}

