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

#include "../_imgui/imgui.h"
#include "../_imgui/imgui_impl_sdl3.h"
#include "../_imgui/imgui_impl_sdlrenderer3.h"

#include "Typedefs.hpp"

using SDL_UniqueWindow   = std::unique_ptr<SDL_Window,   decltype(&SDL_DestroyWindow)  >;
using SDL_UniqueRenderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
using SDL_UniqueTexture  = std::unique_ptr<SDL_Texture,  decltype(&SDL_DestroyTexture) >;

/*==================================================================*/
	#pragma region BasicVideoSpec Singleton Class

class BasicVideoSpec final {
	BasicVideoSpec() noexcept;
	~BasicVideoSpec() noexcept;
	BasicVideoSpec(const BasicVideoSpec&) = delete;
	BasicVideoSpec& operator=(const BasicVideoSpec&) = delete;

	SDL_UniqueWindow   mMainWindow  { nullptr, SDL_DestroyWindow   };
	SDL_UniqueRenderer mMainRenderer{ nullptr, SDL_DestroyRenderer };
	SDL_UniqueTexture  mMainTexture { nullptr, SDL_DestroyTexture  };

public:
	auto getMainWindow() const noexcept { return mMainWindow.get(); }

private:
	s32  mTexPixelPitch{};
	bool enableBuzzGlow{};
	bool enableScanLine{};

	SDL_FRect mOuterFrame{};
	SDL_FRect mInnerFrame{};

	u32  mInnerFrameColor{};
	u32  mOuterFrameColor[2]{};

	s32  mOuterFrameWidth{};
	s32  mFrameScaleMulti{ 2 };

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

	static void showErrorBox(const char* const) noexcept;

	void setBackColor(const u32 color) noexcept {
		mInnerFrameColor = color;
	}

	void setFrameColor(const u32 color_off, const u32 color_on) noexcept {
		mOuterFrameColor[0] = color_off;
		mOuterFrameColor[1] = color_on;
	}

private:
	void createMainWindow(s32, s32);
	void createMainRenderer();
	void createMainTexture(s32, s32);

	void multiplyWindowDimensions();

public:
	bool updateMainTexture(s32, s32);
	void changeTitle(const std::string&);
	void changeFrameMultiplier(s32);

	void raiseWindow();
	void resetWindow();
	void renderPresent();

	void setTextureAlpha(u32);
	void setAspectRatio(s32, s32, s32);

	[[nodiscard]]
	u32* lockTexture();
	void unlockTexture();

	void modifyTexture(const std::span<u32> colorData);

	template <typename T, typename Lambda>
	void modifyTexture(
		const std::span<const T> pixelData,
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
