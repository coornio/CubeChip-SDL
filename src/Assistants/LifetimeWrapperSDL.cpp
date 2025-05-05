/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <SDL3/SDL.h>

#include "LifetimeWrapperSDL.hpp"

/*==================================================================*/

void SDL_Deleter<SDL_Window>     ::deleter(SDL_Window*      ptr) { if (ptr) { SDL_DestroyWindow(ptr); } }
void SDL_Deleter<SDL_Renderer>   ::deleter(SDL_Renderer*    ptr) { if (ptr) { SDL_DestroyRenderer(ptr); } }
void SDL_Deleter<SDL_Texture>    ::deleter(SDL_Texture*     ptr) { if (ptr) { SDL_DestroyTexture(ptr); } }
void SDL_Deleter<SDL_AudioStream>::deleter(SDL_AudioStream* ptr) { if (ptr) { SDL_DestroyAudioStream(ptr); } }
void SDL_Deleter<SDL_DisplayID>  ::deleter(SDL_DisplayID*   ptr) { if (ptr) { SDL_free(ptr); } }
void SDL_Deleter<char>           ::deleter(char*            ptr) { if (ptr) { SDL_free(ptr); } }
void SDL_Deleter<const char>     ::deleter(const char*      ptr) { if (ptr) { SDL_free(const_cast<char*>(ptr)); } }
