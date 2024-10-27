/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <span>
#include <string>
#include <utility>
#include <execution>

#include "../_imgui/imgui.h"
#include "../_imgui/imgui_impl_sdl3.h"
#include "../_imgui/imgui_impl_sdlrenderer3.h"

#include "Typedefs.hpp"
#include "LifetimeWrapperSDL.hpp"

/*==================================================================*/
	#pragma region BasicVideoSpec Singleton Class

class BasicVideoSpec final {
	BasicVideoSpec() noexcept;
	~BasicVideoSpec() noexcept;
	BasicVideoSpec(const BasicVideoSpec&) = delete;
	BasicVideoSpec& operator=(const BasicVideoSpec&) = delete;

	SDL_Unique<SDL_Window>   mMainWindow{};
	SDL_Unique<SDL_Renderer> mMainRenderer{};
	SDL_Unique<SDL_Texture>  mMainTexture{};

	static inline const char* sAppName{};

public:
	auto getMainWindow()  const noexcept { return mMainWindow.get(); }
	auto getMainTexture() const noexcept { return mMainTexture.get(); }

private:
	SDL_FRect mOuterFrame{};
	SDL_FRect mInnerFrame{};

	u32  mOuterFrameColor[2]{};

	s32  mOuterFramePad{};
	s32  mScaleMultiplier{ 2 };

	bool enableBuzzGlow{};
	bool enableScanLine{};

	static bool& errorState() noexcept {
		static bool errorEncountered{};
		return errorEncountered;
	}

public:
	static auto* create(const char* appname = nullptr) noexcept {
		sAppName = appname;
		static BasicVideoSpec self;
		return errorState() ? nullptr : &self;
	}

	static void setErrorState(const bool state) noexcept { errorState() = state; }
	static bool getErrorState()                 noexcept { return errorState();  }

	static void showErrorBox(const char* const) noexcept;

	void setFrameColor(const u32 color_off, const u32 color_on) noexcept {
		mOuterFrameColor[0] = color_off;
		mOuterFrameColor[1] = color_on;
	}

private:
	void multiplyWindowDimensions();

public:
	bool setViewportResolution(const s32 texture_W, const s32 texture_H);

	void resetMainWindow(const s32 window_W = 640, const s32 window_H = 480);
	void setMainWindowTitle(const std::string& title);
	auto getMainWindowID() const noexcept {
		return SDL_GetWindowID(mMainWindow.get());
	}
	void raiseMainWindow();



	void setWindowTitle(SDL_Window* window, const std::string& title);
	void setWindowSize(SDL_Window* window, const s32 window_W, const s32 window_H);
	auto getWindowID(SDL_Window* window) const noexcept {
		return SDL_GetWindowID(window);
	}
	void raiseWindow(SDL_Window* window);



	void setViewportOpacity(const u32 alpha);
	void setAspectRatio(s32, s32, s32);

	void changeScaleMultiplier(const s32 delta);
	void renderPresent();

private:
	[[nodiscard]]
	u32* lockTexture();
	void unlockTexture();

public:
	void modifyTexture(const std::span<u32> colorData);

	template <typename T, typename Lambda>
	void modifyTexture(
		const std::span<const T> pixelData,
		Lambda&& function
	) {
		std::transform(
			//std::execution::unseq,
			pixelData.begin(),
			pixelData.end(),
			lockTexture(),
			function
		);
		unlockTexture();
	}

	template <typename T, typename Lambda>
	void modifyTexture(
		const std::span<const T> pixelData1,
		const std::span<const T> pixelData2,
		Lambda&& function
	) {
		std::transform(
			std::execution::unseq,
			pixelData1.begin(),
			pixelData1.end(),
			pixelData2.begin(),
			lockTexture(),
			function
		);
		unlockTexture();
	}
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
