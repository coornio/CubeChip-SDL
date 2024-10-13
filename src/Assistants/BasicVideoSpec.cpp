/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdexcept>
#include <algorithm>

#include <SDL3/SDL_platform_defines.h>

#ifdef SDL_PLATFORM_WIN32
	#pragma warning(push)
	#pragma warning(disable : 5039)
	#include <dwmapi.h>
	#pragma comment(lib, "Dwmapi")
	#pragma warning(pop)
#endif

#include "BasicVideoSpec.hpp"

/*==================================================================*/
	#pragma region BasicVideoSpec Singleton Class

BasicVideoSpec::BasicVideoSpec() noexcept
	: enableBuzzGlow{ true }
{
	if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		showErrorBox("Failed SDL_INIT_VIDEO");
		return;
	}

	createWindow(0, 0);
	if (getErrorState()) { return; }

	createRenderer();
	if (getErrorState()) { return; }

	resetWindow();
}

BasicVideoSpec::~BasicVideoSpec() noexcept {
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void BasicVideoSpec::createWindow(const s32 window_W, const s32 window_H) {
	quitWindow();

	window = SDL_CreateWindow(nullptr, window_W, window_H, 0);

	if (!window) {
		showErrorBox("Failed to create SDL_Window");
	}
	#ifdef SDL_PLATFORM_WIN32
	else {
		const auto windowHandle{
			SDL_GetPointerProperty(
				SDL_GetWindowProperties(window),
				SDL_PROP_WINDOW_WIN32_HWND_POINTER,
				nullptr
			)
		};

		if (windowHandle) {
			const auto windowRound{ DWMWCP_DONOTROUND };
			DwmSetWindowAttribute(
				static_cast<HWND>(windowHandle),
				DWMWA_WINDOW_CORNER_PREFERENCE,
				&windowRound,
				sizeof(windowRound)
			);
		}
	}
	#endif
}

void BasicVideoSpec::createRenderer() {
	quitRenderer();

	renderer = SDL_CreateRenderer(window, nullptr);
	
	if (!renderer) {
		showErrorBox("Failed to create SDL_Renderer");
	}
}

void BasicVideoSpec::createTexture(s32 texture_W, s32 texture_H) {
	quitTexture();

	texture_W = std::max<s32>(std::abs(texture_W), 1);
	texture_H = std::max<s32>(std::abs(texture_H), 1);

	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		texture_W, texture_H
	);

	if (!texture) {
		showErrorBox("Failed to create SDL_Texture");
	} else {
		SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
		ppitch = texture_W * 4;
	}
}

void BasicVideoSpec::changeTitle(const std::string& name) {
	std::string windowTitle{ "CubeChip :: " + name };
	SDL_SetWindowTitle(window, windowTitle.c_str());
}

void BasicVideoSpec::showErrorBox(const char* const title) noexcept {
	setErrorState(true);
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title,
		SDL_GetError(), nullptr
	);
}

void BasicVideoSpec::raiseWindow() {
	SDL_RaiseWindow(window);
}

void BasicVideoSpec::resetWindow() {
	SDL_SetWindowSize(window, 640, 480);
	changeTitle("Waiting for file...");
	quitTexture();
	renderPresent();
}

u32* BasicVideoSpec::lockTexture() {
	void* pixel_ptr{};
	SDL_LockTexture(
		texture, nullptr,
		&pixel_ptr,
		&ppitch
	);
	return static_cast<u32*>(pixel_ptr);
}
void BasicVideoSpec::unlockTexture() {
	SDL_UnlockTexture(texture);
}

void BasicVideoSpec::modifyTexture(const std::span<u32> colorData) {
	std::move(
		std::execution::unseq,
		colorData.begin(),
		colorData.end(),
		lockTexture()
	);
	unlockTexture();
}

void BasicVideoSpec::setTextureAlpha(const u32 alpha) {
	SDL_SetTextureAlphaMod(texture, static_cast<u8>(alpha));
}

void BasicVideoSpec::setAspectRatio(
	const s32 texture_W,
	const s32 texture_H,
	const s32 padding_S
) {
	const auto padding_A{ std::abs(padding_S) };

	perimeterWidth = padding_A;
	enableScanLine = padding_A == padding_S;

	frameBack = {
		static_cast<f32>(padding_A),
		static_cast<f32>(padding_A),
		static_cast<f32>(texture_W),
		static_cast<f32>(texture_H)
	};

	frameRect.w = texture_W + 2.0f * perimeterWidth;
	frameRect.h = texture_H + 2.0f * perimeterWidth;

	multiplyWindowDimensions();

	SDL_SetRenderLogicalPresentation(
		renderer,
		texture_W + perimeterWidth * 2,
		texture_H + perimeterWidth * 2,
		SDL_LOGICAL_PRESENTATION_INTEGER_SCALE
	);
}

void BasicVideoSpec::multiplyWindowDimensions() {
	const auto window_W{ static_cast<s32>(frameRect.w) };
	const auto window_H{ static_cast<s32>(frameRect.h) };

	SDL_SetWindowMinimumSize(window, window_W, window_H);
	SDL_SetWindowSize(window, window_W * frameMultiplier, window_H * frameMultiplier);
}

void BasicVideoSpec::changeFrameMultiplier(const s32 delta) {
	frameMultiplier = std::clamp(frameMultiplier + delta, 1, 8);
	multiplyWindowDimensions();
}

void BasicVideoSpec::renderPresent() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	if (texture) {
		SDL_SetRenderDrawColor(
			renderer,
			static_cast<u8>(frameRectColor[enableBuzzGlow] >> 16),
			static_cast<u8>(frameRectColor[enableBuzzGlow] >>  8),
			static_cast<u8>(frameRectColor[enableBuzzGlow]), 255
		);
		SDL_RenderFillRect(renderer, &frameRect);

		SDL_SetRenderDrawColor(
			renderer,
			static_cast<u8>(frameBackColor >> 16),
			static_cast<u8>(frameBackColor >>  8),
			static_cast<u8>(frameBackColor), 255
		);
		SDL_RenderFillRect(renderer, &frameBack);

		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		SDL_RenderTexture(renderer, texture, nullptr, &frameBack);

		if (enableScanLine) {
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 32);

			const auto drawLimit{ static_cast<s32>(frameRect.h) };
			for (auto y{ 0 }; y < drawLimit; y += perimeterWidth) {
				SDL_RenderLine(
					renderer,
					frameRect.x, static_cast<f32>(y),
					frameRect.w, static_cast<f32>(y)
				);
			}
		}
	} else {
		SDL_RenderTexture(renderer, nullptr, nullptr, nullptr);
	}

	SDL_RenderPresent(renderer);
}

void BasicVideoSpec::quitTexture() noexcept {
	if (texture) {
		SDL_DestroyTexture(texture);
		texture = nullptr;
	}
}
void BasicVideoSpec::quitRenderer() noexcept {
	if (renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}
}
void BasicVideoSpec::quitWindow() noexcept {
	if (window) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
