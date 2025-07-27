/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <utility>
#include <optional>
#include <algorithm>

#include "Typedefs.hpp"
#include "TripleBuffer.hpp"
#include "LifetimeWrapperSDL.hpp"
#include "SettingWrapper.hpp"

/*==================================================================*/

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
			: W{ W < 0 ? 0 : W }
			, H{ H < 0 ? 0 : H }
		{}

		auto frect() const noexcept -> SDL_FRect;

		constexpr operator s32() const noexcept { return W * H; }
	};

	struct Viewport {
		Rect rect{};
		s32 mult{}, ppad{};

		constexpr Viewport(s32 W = 0, s32 H = 0, s32 mult = 0, s32 ppad = 0) noexcept
			: rect{ std::clamp(W, 0x0, 0xFFF), std::clamp(H, 0x0, 0xFFF) }
			, mult{ std::clamp(mult, 0x1, 0xF) }
			, ppad{ std::clamp(ppad, 0x0, 0xF) }
		{}

		constexpr auto rotate_if(bool cond) const noexcept {
			return cond
				? Viewport{ rect.H, rect.W, mult, ppad }
				: Viewport{ rect.W, rect.H, mult, ppad };
		}

		constexpr auto scaled() const noexcept {
			return Rect{ rect.W * mult, rect.H * mult };
		}
		constexpr auto padded() const noexcept {
			return Rect{ rect.W * mult + ppad * 2, rect.H * mult + ppad * 2 };
		}

		static constexpr auto pack(s32 W, s32 H, s32 mult, s32 ppad) noexcept {
			return ((u32(W)     & 0xFFFu) << 00u) |
				   ((u32(H)     & 0xFFFu) << 12u) |
				   ((u32(mult)  & 0xFu)   << 24u) |
				   ((u32(ppad)  & 0xFu)   << 28u);
		}

		static constexpr auto unpack(u32 packed) noexcept {
			return Viewport{
				s32((packed >> 00u) & 0xFFFu),
				s32((packed >> 12u) & 0xFFFu),
				s32((packed >> 24u) & 0xFu),
				s32((packed >> 28u) & 0xFu)
			};
		}

		auto frect() const noexcept -> SDL_FRect;
	};

private:
	SDL_Unique<SDL_Window>   mMainWindow{};
	SDL_Unique<SDL_Renderer> mMainRenderer{};
	SDL_Unique<SDL_Texture>  mWindowTexture{};
	SDL_Unique<SDL_Texture>  mSystemTexture{};

	auto getTextureSizeRect(SDL_Texture* texture) const noexcept -> Rect;

/*==================================================================*/

	Viewport  mCurViewport{};
	Atom<u32> mNewViewport{};

	Atom<u32> mOutlineColor{};
	Atom<u8>  mTextureAlpha{ 0xFF };

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

	SDL_Window* getMainWindow() const noexcept
		{ return mMainWindow; }

	SDL_Renderer* getMainRenderer() const noexcept
		{ return mMainRenderer; }

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
	void prepareWindowTexture();
	void prepareSystemTexture();
	void renderViewport();

public:
	void setViewportAlpha(u32 alpha) noexcept;

	/**
	 * @brief Sets various parameters to shape, scale, and pad the system's Viewport. Thread-safe.
	 * @param[in] W :: The width of the output texture in pixels.
	 * @param[in] H :: The height of the output texture in pixels.
	 * @param[in] mult :: Integer multiplier of the texture size to adjust minimum size. Capped at 16.
	 * @param[in] pad :: The thickness of the padding (colorable, see 'setOutlineColor')
	 * surrounding the Viewport in pixels. When negative, doubles as a flag that controls whether
	 * to draw scanlines over the Viewport. Capped at -16..16.
	 * @return Boolean if successful.
	 */
	void setViewportSizes(s32 W, s32 H, s32 mult = 0, s32 ppad = 0) noexcept;
	auto getViewportSizes() const noexcept -> Viewport;

public:
	void resetMainWindow();
	void setMainWindowTitle(const Str& title, const Str& desc);
	bool isMainWindowID(u32 id) const noexcept;
	void raiseMainWindow();

	void renderPresent(bool core, const char* overlay_data);
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
