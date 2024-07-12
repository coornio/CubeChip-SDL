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

#include "../Types.hpp"

class BasicVideoSpec {
	SDL_Window*   window{};
	SDL_Renderer* renderer{};
	SDL_Texture*  texture{};

	SDL_FRect frameGame{};
	SDL_FRect frameFull{};
	SDL_Color backFrameColor{};
	SDL_Color PerimeterColor{};

	s32 PerimeterWidth{};
	s32 frameMultiplier{ 2 };
	s32 ppitch{};

	bool enableBuzzGlow{};
	bool enableScanLine{};

public:
	explicit BasicVideoSpec();
	~BasicVideoSpec();

	static bool showErrorBoxSDL(std::string_view);
	static bool showErrorBox(std::string_view, std::string_view);

private:
	void createWindow(s32, s32);
	void createRenderer();

public:
	void createTexture(s32, s32);
	void changeTitle(const char* = nullptr);

	void raiseWindow();
	void resetWindow();

	[[nodiscard]]
	u32* lockTexture();
	void unlockTexture();
	void renderPresent();

	void setTextureAlpha(usz);
	void setAudioLineColor(u32, u32);
	void setBackgroundColor(u32);
	void setAspectRatio(s32, s32, s32);

private:
	void multiplyWindowDimensions();

public:
	void changeFrameMultiplier(s32);

private:
	void quitWindow();
	void quitRenderer();
	void quitTexture();
};
