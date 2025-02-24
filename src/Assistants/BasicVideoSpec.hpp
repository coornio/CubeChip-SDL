/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <utility>
#include <execution>

#include <SDL3/SDL.h>

#include "Typedefs.hpp"
#include "Concepts.hpp"
#include "TripleBuffer.hpp"
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
	SDL_Unique<SDL_Texture>  mOuterTexture{};
	SDL_Unique<SDL_Texture>  mInnerTexture{};

	static inline const char* sAppName{};
	static inline bool mSuccessful{ true };

	SDL_FRect mOuterFrame{};
	SDL_FRect mInnerFrame{};
	SDL_FRect mTextureFrame{};

	struct Rect {
		s32 W{}, H{};
		constexpr operator s32() const noexcept { return W * H; }
	};

	Atom<std::shared_ptr<Rect>> mTextureSize{};

	Atom<u32> mOuterFrameColor[2]{};

	Atom<s32> mTextureScale{};
	Atom<s32> mFramePadding{};

	Atom<u8>  mTextureAlpha{ 0xFF };
	Atom<bool> mNewTextureNeeded{};

	bool mEnableBuzzGlow{};
	bool mEnableScanline{};

	s32  mViewportPadding{};
	s32  mViewportRotation{};

public:
	TripleBuffer<u32> displayBuffer;

	static auto* create(const char* appName) noexcept {
		sAppName = appName;
		static BasicVideoSpec self;
		return mSuccessful ? &self : nullptr;
	}

	static bool isSuccessful() noexcept { return mSuccessful; }

	static void showErrorBox(const char* const title) noexcept;

	static auto getDisplayWidth(u32 displayID) noexcept {
		const auto displayMode{ SDL_GetCurrentDisplayMode(displayID) };
		return displayMode ? displayMode->w : 0;
	}
	static auto getDisplayHeight(u32 displayID) noexcept {
		const auto displayMode{ SDL_GetCurrentDisplayMode(displayID) };
		return displayMode ? displayMode->h : 0;
	}

	void setFrameColor(u32 color_off, u32 color_on) noexcept;

	template <typename T, size_type N>
	void scaleInterface(T(&appFont)[N]) {
		updateInterfacePixelScaling(appFont, static_cast<s32>(N),
			SDL_GetWindowDisplayScale(mMainWindow));
	}

	void processInterfaceEvent(SDL_Event* event) const noexcept;

private:
	void updateInterfacePixelScaling(const void* fontData, s32 fontSize, f32 newScale);
	void prepareViewport();
	void renderViewport(SDL_Texture* windowTexture);

public:
	void setViewportAlpha(u32 alpha);
	void setViewportSizes(s32 texture_W, s32 texture_H, s32 upscale_M = 0, s32 padding_S = 0);

public:
	void resetMainWindow(s32 window_W = 640, s32 window_H = 480);
	void setMainWindowTitle(const Str& title);
	auto getMainWindowID() const noexcept {
		return SDL_GetWindowID(mMainWindow);
	}
	void raiseMainWindow();

	void setWindowTitle(SDL_Window* window, const Str& title);
	void setWindowSize(SDL_Window* window, s32 window_W, s32 window_H);
	auto getWindowID(SDL_Window* window) const noexcept {
		return SDL_GetWindowID(window);
	}
	void raiseWindow(SDL_Window* window);

	void renderPresent(const char* const stats);
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
