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

/*==================================================================*/
	#pragma region BasicVideoSpec Singleton Class
/*==================================================================*/

class BasicVideoSpec final {
	BasicVideoSpec() noexcept;
	~BasicVideoSpec() noexcept;
	BasicVideoSpec(const BasicVideoSpec&) = delete;
	BasicVideoSpec& operator=(const BasicVideoSpec&) = delete;

	SDL_Window*   window{};
	SDL_Renderer* renderer{};
	SDL_Texture*  texture{};

	s32  ppitch{};
	bool enableBuzzGlow{};
	bool enableScanLine{};

	bool errorEncountered{};

	SDL_FRect frameBack{};
	SDL_FRect frameRect{};

	u32  frameBackColor{};
	u32  frameRectColor[2]{};

	s32  perimeterWidth{};
	s32  frameMultiplier{ 2 };

public:
	static auto& create() noexcept {
		static BasicVideoSpec self;
		return self;
	}

	void setErrorState(const bool state) noexcept { errorEncountered = state; }
	bool getErrorState()           const noexcept { return errorEncountered; }

	static bool showErrorBox(const char* const) noexcept;
	static bool showErrorBox(const char* const, const char* const) noexcept;

	void setBackColor (
		const u32 color
	) noexcept {
		frameBackColor = color;
	}

	void setFrameColor(
		const u32 color_off,
		const u32 color_on
	) noexcept {
		frameRectColor[0] = color_off;
		frameRectColor[1] = color_on;
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

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

extern BasicVideoSpec& BVS;
