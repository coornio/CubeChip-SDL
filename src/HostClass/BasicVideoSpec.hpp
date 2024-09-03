/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL3/SDL.h>

#include <span>
#include <string>
#include <utility>
#include <execution>

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

	SDL_FRect frameBack{};
	SDL_FRect frameRect{};

	u32  frameBackColor{};
	u32  frameRectColor[2]{};

	s32  perimeterWidth{};
	s32  frameMultiplier{ 2 };

	static bool& errorState() noexcept {
		static bool errorEncountered{};
		return errorEncountered;
	}

public:
	static auto* create() noexcept {
		static BasicVideoSpec self;
		return errorState() ? nullptr : &self;
	}

	static void setErrorState(const bool state) noexcept { errorState() = state; }
	static bool getErrorState()                 noexcept { return errorState();  }

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
	void modifyTexture(const std::span<u32> colorData);

	template <typename T, typename Lambda>
	void modifyTexture(const std::span<T> pixelData, Lambda&& function) {
		std::transform(
			std::execution::unseq,
			pixelData.begin(),
			pixelData.end(),
			lockTexture(),
			function
		);
		unlockTexture();
	}

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
