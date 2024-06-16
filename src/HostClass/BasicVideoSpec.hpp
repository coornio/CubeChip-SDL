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

	SDL_FRect frameGame{};
	SDL_FRect frameFull{};
	SDL_Color PerimeterColor{};
	Sint32    PerimeterWidth{};
	Sint32    frameMultiplier{ 2 };
	Sint32    ppitch{};

	bool      enableBuzzGlow{};
	bool      enableScanLine{};

public:
	Uint32*   pixels{};

public:
	explicit BasicVideoSpec();
	~BasicVideoSpec();

	static bool showErrorBoxSDL(std::string_view);
	static bool showErrorBox(std::string_view, std::string_view);

private:
	void createWindow(Sint32, Sint32);
	void createRenderer();

public:
	void createTexture(Sint32 = 0, Sint32 = 0);
	void changeTitle(const char* = nullptr);

	void raiseWindow();
	void resetWindow();
	void lockTexture();
	void unlockTexture();
	void renderPresent();

	void setTextureAlpha(std::size_t);
	void AudioOutline(Uint32, Uint32);
	void setAspectRatio(Sint32, Sint32, Sint32);

private:
	void multiplyWindowDimensions();

public:
	void changeFrameMultiplier(Sint32);

private:
	void quitWindow();
	void quitRenderer();
	void quitTexture();
};
