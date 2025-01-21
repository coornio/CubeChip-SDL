/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include <utility>
#include <concepts>
#include <execution>

#include <SDL3/SDL.h>

#include "Typedefs.hpp"
#include "Concepts.hpp"
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
	static inline bool mSuccessful{ true };

	SDL_FRect mOuterFrame{};
	SDL_FRect mInnerFrame{};

	RGBA mOuterFrameColor[2]{};

	bool enableBuzzGlow{};
	bool enableScanLine{};

public:
	static auto* create(const char* appName) noexcept {
		sAppName = appName;
		static BasicVideoSpec self;
		return mSuccessful ? &self : nullptr;
	}

	static bool isSuccessful() noexcept { return mSuccessful; }

	static void showErrorBox(const char* const title) noexcept;

	static auto getDisplayWidth(const u32 displayID) noexcept {
		const auto displayMode{ SDL_GetCurrentDisplayMode(displayID) };
		return displayMode ? displayMode->w : 0;
	}
	static auto getDisplayHeight(const u32 displayID) noexcept {
		const auto displayMode{ SDL_GetCurrentDisplayMode(displayID) };
		return displayMode ? displayMode->h : 0;
	}

	void setFrameColor(const u32 color_off, const u32 color_on) noexcept {
		mOuterFrameColor[0] = color_off;
		mOuterFrameColor[1] = color_on;
	}

	template <typename T, std::size_t N = std::dynamic_extent>
	void scaleInterface(std::span<const T, N> appFont) {
		updateInterfacePixelScaling(
			appFont.data(), static_cast<s32>(appFont.size()),
			SDL_GetWindowDisplayScale(mMainWindow)
		);
	}

	void processInterfaceEvent(SDL_Event* event) const noexcept;

private:
	void updateInterfacePixelScaling(const void* fontData, const s32 fontSize, const f32 newScale);
	void drawViewportTexture(SDL_Texture* viewportTexture);

public:
	void setViewportOpacity(const u32 alpha);
	bool setViewportDimensions(
		const s32 texture_W, const s32 texture_H
	);
	bool setViewportDimensions(
		const s32 texture_W, const s32 texture_H,
		const s32 upscale_M, const s32 padding_S
	);

	void resetMainWindow(const s32 window_W = 640, const s32 window_H = 480);
	void setMainWindowTitle(const Str& title);
	auto getMainWindowID() const noexcept {
		return SDL_GetWindowID(mMainWindow.get());
	}
	void raiseMainWindow();

	void setWindowTitle(SDL_Window* window, const Str& title);
	void setWindowSize(SDL_Window* window, const s32 window_W, const s32 window_H);
	auto getWindowID(SDL_Window* window) const noexcept {
		return SDL_GetWindowID(window);
	}
	void raiseWindow(SDL_Window* window);

	void renderPresent(const char* const stats);

private:
	[[nodiscard]]
	u32* lockTexture();
	void unlockTexture();

public:
	template <IsContiguousContainer T>
	void modifyTexture(const T& pixelData) {
		std::copy(
			std::execution::unseq,
			pixelData.begin(),
			pixelData.end(),
			lockTexture()
		);
		unlockTexture();
	}

	template <IsContiguousContainer T, typename Lambda>
	void modifyTexture(
		const T& pixelData,
		Lambda&& function
	) {
		std::transform(
			std::execution::unseq,
			pixelData.begin(),
			pixelData.end(),
			lockTexture(),
			function
		);
		unlockTexture();
	}

	template <IsContiguousContainer T, typename Lambda>
	void modifyTexture(
		const T& pixelData1,
		const T& pixelData2,
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
