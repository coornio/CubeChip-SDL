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

	struct Rect {
		s32 W{}, H{};

		constexpr auto frect() const noexcept {
			return SDL_FRect{
				static_cast<f32>(0),
				static_cast<f32>(0),
				static_cast<f32>(W),
				static_cast<f32>(H)
			};
		}

		constexpr operator s32() const noexcept { return W * H; }
	};

	struct Viewport {
		Rect rect{};
		s32 scale{}, pad{};

		Viewport(s32 W = 0, s32 H = 0, s32 scale = 0, s32 pad = 0)
			: rect{ W, H }, scale{ std::max(1, scale) }, pad{ std::abs(pad) }
		{}
		Viewport(Rect rect, s32 scale = 1, s32 pad = 0)
			: rect{ rect }, scale{ std::max(1, scale) }, pad{ std::abs(pad) }
		{}

		constexpr Viewport rotate_if(bool cond) const noexcept {
			return cond ? Viewport{ rect.H, rect.W, scale, pad } : Viewport{ rect.W, rect.H, scale, pad };
		}

		constexpr auto scaled() const noexcept {
			return Rect{ rect.W * scale, rect.H * scale };
		}
		constexpr auto padded() const noexcept {
			return Rect{ rect.W * scale + pad * 2, rect.H * scale + pad * 2 };
		}

		constexpr auto frect() const noexcept {
			return SDL_FRect{
				static_cast<f32>(pad),
				static_cast<f32>(pad),
				static_cast<f32>(rect.W * scale),
				static_cast<f32>(rect.H * scale)
			};
		}
	};

	SDL_Unique<SDL_Window>   mMainWindow{};
	SDL_Unique<SDL_Renderer> mMainRenderer{};
	SDL_Unique<SDL_Texture>  mOuterTexture{};
	SDL_Unique<SDL_Texture>  mInnerTexture{};

	static inline const char* sAppName{};
	static inline bool mSuccessful{ true };

	Viewport mViewportFrame{};

	AtomSharedPtr<Rect> mTextureSize{};
	Atom<u32> mOutlineColor{};
	Atom<s32> mTextureScale{};
	Atom<s32> mFramePadding{};
	Atom<u8>  mTextureAlpha{ 0xFF };
	Atom<bool> mNewTextureNeeded{};

	bool mEnableScanline{};

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

	void setOutlineColor(u32 color) noexcept;

	template <typename T, size_type N>
	void scaleInterface(T(&appFont)[N]) {
		updateInterfacePixelScaling(appFont, static_cast<s32>(N),
			SDL_GetWindowDisplayScale(mMainWindow));
	}

	void processInterfaceEvent(SDL_Event* event) const noexcept;

	void rotateViewport(s32 delta) noexcept {
		mViewportRotation += delta;
		mViewportRotation &= 3;
	}
	void setViewportRotation(s32 value) noexcept {
		mViewportRotation = value & 3;
	}

private:
	void updateInterfacePixelScaling(const void* fontData, s32 fontSize, f32 newScale);
	void renderViewport();

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
