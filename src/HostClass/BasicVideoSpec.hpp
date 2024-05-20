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

public:
	explicit BasicVideoSpec(Sint32, Sint32);
	~BasicVideoSpec();

	static bool showErrorBoxSDL(std::string_view);
	static bool showErrorBox(std::string_view, std::string_view);

	void changeTitle(const char* = nullptr);
	void createWindow(Sint32, Sint32);
	void createRenderer();
	void createTexture(Sint32 = 0, Sint32 = 0);

	float   aspect{};
	Sint32  ppitch{};
	Uint32* pixels{};
	double  framerate{ 60.0 };

	void lockTexture();
	void unlockTexture();
	void renderPresent();

	void resizeWindow(Sint32 = 0, Sint32 = 0);

	void setTextureAlpha(std::size_t);
	void setTextureBlend(SDL_BlendMode);
	void setAspectRatio(std::pair<Sint32, Sint32>);

	void quitWindow();
	void quitRenderer();
	void quitTexture();
};
