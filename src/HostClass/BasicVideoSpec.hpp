/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#pragma warning(push)
#pragma warning(disable : 26819) // C fallthrough warning disabled
#include <SDL3/SDL.h>
#pragma warning(pop)

#include <string>
#include <utility>

class BasicVideoSpec {
	SDL_Window*   window{};
	SDL_Renderer* renderer{};
	SDL_Texture*  texture{};

	const std::string emuName;
	const std::string emuVersion;
	      std::string windowTitle;

	SDL_FRect frameGame{};
	SDL_FRect frameFull{};
	SDL_Color frameColor{};
	Sint32    frameWidth{};
	Sint32    ppitch{};

	bool      visualBeep{};
	bool      scanLineOn{};

public:
	Uint32*   pixels{};

public:
	explicit BasicVideoSpec(Sint32, Sint32);
	~BasicVideoSpec();

	static bool showErrorBoxSDL(std::string_view);
	static bool showErrorBox(std::string_view, std::string_view);

	void changeTitle(const char* = nullptr);
	void createWindow(Sint32, Sint32);
	void createRenderer();
	void createTexture(Sint32 = 0, Sint32 = 0);

	void raiseWindow();
	void lockTexture();
	void unlockTexture();
	void renderPresent();

	void resizeWindow(Sint32 = 0, Sint32 = 0);

	void setTextureAlpha(std::size_t);
	void AudioOutline(Uint32, Uint32);
	void setAspectRatio(Sint32, Sint32, Sint32);

	void quitWindow();
	void quitRenderer();
	void quitTexture();
};
