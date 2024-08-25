/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <utility>

#include "../Types.hpp"

class BasicVideoSpec final {
	SDL_Window*   window{};
	SDL_Renderer* renderer{};
	SDL_Texture*  texture{};

	s32  ppitch{};
	bool enableBuzzGlow{};
	bool enableScanLine{};

	SDL_FRect frameGame{};
	SDL_FRect frameFull{};

	u32  frameGameColor{};
	u32  frameFullColor[2]{};

	s32  perimeterWidth{};
	s32  frameMultiplier{ 2 };

public:
	explicit BasicVideoSpec();
	~BasicVideoSpec();

	static bool showErrorBoxSDL(std::string_view);
	static bool showErrorBox(std::string_view, std::string_view);

	void setBackColor (
		const u32 color
	) noexcept {
		frameGameColor = color;
	}

	void setFrameColor(
		const u32 color_off,
		const u32 color_on
	) noexcept {
		frameFullColor[0] = color_off;
		frameFullColor[1] = color_on;
	}


private:
	void createWindow(s32, s32);
	void createRenderer();

public:
	void createTexture(s32, s32);
	void changeTitle(const std::string&);

	void raiseWindow();
	void resetWindow();
	void renderPresent();

	[[nodiscard]]
	u32* lockTexture();
	void unlockTexture();

	void setTextureAlpha(usz);
	void setAspectRatio(s32, s32, s32);

private:
	void multiplyWindowDimensions();

public:
	void changeFrameMultiplier(s32);

private:
	void quitWindow() noexcept;
	void quitRenderer() noexcept;
	void quitTexture() noexcept;
};
