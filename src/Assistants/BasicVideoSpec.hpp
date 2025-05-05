/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <utility>
#include <optional>
#include <execution>

#include "TripleBuffer.hpp"
#include "LifetimeWrapperSDL.hpp"
#include "SettingWrapper.hpp"

union  SDL_Event;

struct SDL_Rect;
struct SDL_FRect;

/*==================================================================*/
	#pragma region BasicVideoSpec Singleton Class

class BasicVideoSpec final {

public:
	struct Rect {
		s32 W{}, H{};

		constexpr Rect() noexcept = default;
		constexpr Rect(s32 W, s32 H) noexcept
			: W{ W }, H{ H }
		{}

		auto frect() const noexcept -> SDL_FRect;

		constexpr operator s32() const noexcept { return W * H; }
	};

	struct Viewport {
		Rect rect{};
		s32 scale{}, pad{};

		constexpr Viewport(s32 W = 0, s32 H = 0, s32 scale = 0, s32 pad = 0) noexcept
			: rect{ W, H }, scale{ std::max(1, scale) }, pad{ std::max(0, pad) }
		{}
		constexpr Viewport(Rect rect, s32 scale = 1, s32 pad = 0) noexcept
			: rect{ rect }, scale{ std::max(1, scale) }, pad{ std::max(0, pad) }
		{}

		constexpr auto rotate_if(bool cond) const noexcept {
			return cond
				? Viewport{ rect.H, rect.W, scale, pad }
				: Viewport{ rect.W, rect.H, scale, pad };
		}

		constexpr auto scaled() const noexcept {
			return Rect{ rect.W * scale, rect.H * scale };
		}
		constexpr auto padded() const noexcept {
			return Rect{ rect.W * scale + pad * 2, rect.H * scale + pad * 2 };
		}

		auto frect() const noexcept -> SDL_FRect;
	};

private:
	SDL_Unique<SDL_Window>   mMainWindow{};
	SDL_Unique<SDL_Renderer> mMainRenderer{};
	SDL_Unique<SDL_Texture>  mOuterTexture{};
	SDL_Unique<SDL_Texture>  mInnerTexture{};

/*==================================================================*/

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
	s32 mViewportScaleMode{};

public:
	TripleBuffer<u32> displayBuffer;

	struct Settings {
		struct Window {
			s32 x{}, y{}, w{}, h{};
			operator SDL_Rect() const noexcept;
		};

		static constexpr Window
			defaults{ 0, 0, 640, 480 };

		Window window{ defaults };
		struct Viewport {
			s32  filtering{ 0 };
			bool int_scale{ true };
			bool scanlines{ true };
		} viewport;
		bool first_run{ true };

		auto map() noexcept {
			return SettingsMap{
				makeSetting("VIDEO.Window.X", &window.x),
				makeSetting("VIDEO.Window.Y", &window.y),
				makeSetting("VIDEO.Window.W", &window.w),
				makeSetting("VIDEO.Window.H", &window.h),
				makeSetting("VIDEO.Window.FirstRun", &first_run),
				makeSetting("VIDEO.Viewport.Filtering", &viewport.filtering),
				makeSetting("VIDEO.Viewport.Int_Scale", &viewport.int_scale),
				makeSetting("VIDEO.Viewport.Scanlines", &viewport.scanlines),
			};
		}
	};

	[[nodiscard]]
	auto exportSettings() const noexcept -> Settings;

private:
	BasicVideoSpec(const Settings& settings) noexcept;
	~BasicVideoSpec() noexcept;
	BasicVideoSpec(const BasicVideoSpec&) = delete;
	BasicVideoSpec& operator=(const BasicVideoSpec&) = delete;

/*==================================================================*/

	static inline bool mSuccessful{ true };

public:
	static auto* initialize(const Settings& settings) noexcept {
		static BasicVideoSpec self(settings);
		return mSuccessful ? &self : nullptr;
	}

	static bool isSuccessful() noexcept { return mSuccessful; }
	static void showErrorBox(const char* const title) noexcept;

/*==================================================================*/

private:
	auto computeOverlapArea(const SDL_Rect& a, const SDL_Rect& b) noexcept;

	auto squaredDistance(s32 x1, s32 y1, s32 x2, s32 y2) noexcept;

public:
	void normalizeRectToDisplay(SDL_Rect& rect, SDL_Rect& deco, bool first_run) noexcept;

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
	void setViewportScaleMode(s32 mode) noexcept;
	void cycleViewportScaleMode() noexcept;
	void setBorderColor(u32 color) noexcept;

/*==================================================================*/

private:
	void scaleInterface(const void* data, size_type size);

public:
	template <typename T, std::size_t N>
	void scaleInterface(T(&appFont)[N]) {
		scaleInterface(appFont, N);
	}

	void processInterfaceEvent(SDL_Event* event) const noexcept;

/*==================================================================*/

private:
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
	bool isMainWindowID(u32 id) const noexcept;
	void raiseMainWindow();

	void renderPresent(const char* overlay_data);
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
