/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <utility>
#include <optional>
#include <execution>

#include <SDL3/SDL.h>

#include "Typedefs.hpp"
#include "Concepts.hpp"
#include "TripleBuffer.hpp"
#include "LifetimeWrapperSDL.hpp"

/*==================================================================*/
	#pragma region BasicVideoSpec Singleton Class

class BasicVideoSpec final {

	struct Rect {
		s32 W{}, H{};

		constexpr Rect() noexcept {}
		constexpr Rect(s32 W, s32 H) noexcept
			: W{ W }, H{ H }
		{}

		auto frect() const noexcept {
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

		constexpr Viewport(s32 W = 0, s32 H = 0, s32 scale = 0, s32 pad = 0)
			: rect{ W, H }, scale{ std::max(1, scale) }, pad{ std::max(0, pad) }
		{}
		constexpr Viewport(Rect rect, s32 scale = 1, s32 pad = 0)
			: rect{ rect }, scale{ std::max(1, scale) }, pad{ std::max(0, pad) }
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

		auto frect() const noexcept {
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

/*==================================================================*/

	static constexpr SDL_Rect cDefaultWindow{ 240, 240, 640, 480 };

	Viewport mViewportFrame{};

	AtomSharedPtr<Rect> mTextureSize{};
	Atom<u32> mOutlineColor{};
	Atom<s32> mTextureScale{};
	Atom<s32> mFramePadding{};
	Atom<u8>  mTextureAlpha{ 0xFF };
	Atom<bool> mNewTextureNeeded{};

	bool mUsingScanlines{};
	bool mIntegerScaling{};

	s32 mViewportRotation{};
	s32 mViewportScaleMode{ SDL_SCALEMODE_NEAREST };

public:
	TripleBuffer<u32> displayBuffer;

	struct Settings {
		s32 windowPosX{};
		s32 windowPosY{};
		s32 windowSizeX{};
		s32 windowSizeY{};
		s32 viewportScaleMode{};
		bool integerScaling{};
		bool usingScanlines{};
	};

	[[nodiscard]]
	Settings exportSettings() const noexcept {
		Settings out;

		SDL_GetWindowPosition(mMainWindow, &out.windowPosX,  &out.windowPosY );
		SDL_GetWindowSize    (mMainWindow, &out.windowSizeX, &out.windowSizeY);
		out.viewportScaleMode = mViewportScaleMode;
		out.integerScaling    = mIntegerScaling;
		out.usingScanlines    = mUsingScanlines;
	
		return out;
	}

private:
	BasicVideoSpec(const Settings& settings) noexcept;
	~BasicVideoSpec() noexcept;
	BasicVideoSpec(const BasicVideoSpec&) = delete;
	BasicVideoSpec& operator=(const BasicVideoSpec&) = delete;

/*==================================================================*/

	static inline bool mSuccessful{ true };

public:
	static auto* create(const Settings& settings) noexcept {
		static BasicVideoSpec self(settings);
		return mSuccessful ? &self : nullptr;
	}

	static bool isSuccessful() noexcept { return mSuccessful; }
	static void showErrorBox(const char* const title) noexcept;

	static auto getCurrentDisplaySize(u32 displayID) noexcept {
		const auto displayMode{ SDL_GetCurrentDisplayMode(displayID) };
		return Rect{ displayMode->w, displayMode->h };
	}

/*==================================================================*/

private:
	auto computeOverlapArea(const SDL_Rect& a, const SDL_Rect& b) {
		auto xOverlap{ std::max(0, std::min(a.x + a.w, b.x + b.w) - std::max(a.x, b.x)) };
		auto yOverlap{ std::max(0, std::min(a.y + a.h, b.y + b.h) - std::max(a.y, b.y)) };
		return xOverlap * yOverlap;
	}

	auto squaredDistance(s32 x1, s32 y1, s32 x2, s32 y2) {
		s64 dx{ x1 - x2 };
		s64 dy{ y1 - y2 };
		return dx * dx + dy * dy;
	}

public:
	void normalizeRectToDisplay(SDL_Rect& rect) noexcept;

/*==================================================================*/

	bool isIntegerScaling()     const noexcept { return mIntegerScaling; }
	void isIntegerScaling(bool state) noexcept { mIntegerScaling = state; }
	void toggleIntegerScaling()       noexcept { mIntegerScaling = !mIntegerScaling; }

	bool isUsingScanlines()     const noexcept { return mUsingScanlines; }
	void isUsingScanlines(bool state) noexcept { mUsingScanlines = state; }
	void toggleUsingScanlines()       noexcept { mUsingScanlines = !mUsingScanlines; }

	void rotateViewport(s32 delta) noexcept {
		mViewportRotation += delta;
		mViewportRotation &= 3;
	}
	void setViewportRotation(s32 value) noexcept {
		mViewportRotation = value & 3;
	}

	auto getViewportScaleMode() const noexcept { return mViewportScaleMode; }
	void setViewportScaleMode(SDL_ScaleMode mode) noexcept {
		if (mode != SDL_SCALEMODE_INVALID) {
			mViewportScaleMode = mode;
			SDL_SetTextureScaleMode(mOuterTexture, mode);
		}
	}
	void cycleViewportScaleMode() noexcept {
		switch (mViewportScaleMode) {
			case SDL_SCALEMODE_NEAREST:
				setViewportScaleMode(SDL_SCALEMODE_LINEAR);
				break;
			case SDL_SCALEMODE_LINEAR:
				setViewportScaleMode(SDL_SCALEMODE_PIXELART);
				break;
			default:
				setViewportScaleMode(SDL_SCALEMODE_NEAREST);
				break;
		}
	}
	void setBorderColor(u32 color) noexcept;

/*==================================================================*/

	template <typename T, size_type N>
	void scaleInterface(T(&appFont)[N]) {
		updateInterfacePixelScaling(appFont, static_cast<s32>(N),
			SDL_GetWindowDisplayScale(mMainWindow));
	}

	void processInterfaceEvent(SDL_Event* event) const noexcept;

/*==================================================================*/

private:
	void updateInterfacePixelScaling(const void* fontData, s32 fontSize, f32 newScale);
	void renderViewport();

public:
	void setViewportAlpha(u32 alpha);

	/**
	 * @brief Sets various parameters to shape, scale, and pad the system's Viewport. Thread-safe.
	 * @param[in] texture_W :: The width of the output texture in pixels.
	 * @param[in] texture_H :: The height of the output texture in pixels.
	 * @param[in] upscale_M :: Integer multiplier of the texture size to adjust minimum size. Capped at 16.
	 * @param[in] padding_S :: The thickness of the padding (colorable, see 'setOutlineColor')
	 * surrounding the Viewport in pixels. When negative, doubles as a flag that controls whether
	 * to draw scanlines over the Viewport. Capped at -16..16.
	 * @return Boolean if successful.
	 */
	void setViewportSizes(s32 texture_W, s32 texture_H, s32 upscale_M = 0, s32 padding_S = 0);

public:
	void resetMainWindow();
	void setMainWindowTitle(const Str& title, const Str& desc);
	auto getMainWindowID() const noexcept {
		return SDL_GetWindowID(mMainWindow);
	}
	void raiseMainWindow();

	void renderPresent(const char* const stats);
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
