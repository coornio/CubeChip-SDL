/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#pragma warning(push)
#pragma warning(disable : 26819) // C fallthrough warning disabled
#include <SDL_video.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#pragma warning(pop)

class BasicRenderer {
	SDL_Window*   window{};
	SDL_Renderer* renderer{};
	SDL_Texture*  texture{};

	const std::string emuName    { "CubeChip"s   };
	const std::string emuVersion { "[22.03.24]"s };
		  std::string windowTitle{};

	void errorMessage(std::string&&);

public:
	BasicRenderer();

	void changeTitle(std::string_view);
	bool createWindow();
	bool createRenderer();
	bool createTexture(const s32, const s32);

	s32 window_W;
	s32 window_H;

	float aspect{};
	s32   ppitch{};
	u32*  pixels{};

	void lockTexture();
	void unlockTexture();
	void renderPresent();

	void getWindowSize(const bool);

	void setTextureAlpha(usz);
	void setTextureBlend(SDL_BlendMode);
	void setAspectRatio(float);

	void quitWindow();
	void quitRenderer();
	void quitTexture();
	void quitSDL();
};
