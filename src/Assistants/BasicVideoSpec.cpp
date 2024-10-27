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
		showErrorBox("Failed to init SDL video!");
		return;
	}

	mMainWindow = SDL_CreateWindow(sAppName, 0, 0, SDL_WINDOW_HIDDEN);
	if (!mMainWindow) {
		showErrorBox("Failed to create Main window!");
		return;
	}
	#if defined(SDL_PLATFORM_WIN32) && !defined(OLD_WINDOWS_SDK)
	else {
		const auto windowHandle{
			SDL_GetPointerProperty(
				SDL_GetWindowProperties(mMainWindow),
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

	mMainRenderer = SDL_CreateRenderer(mMainWindow, nullptr);
	if (!mMainRenderer) {
		showErrorBox("Failed to create Main renderer!");
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	ImGui_ImplSDL3_InitForSDLRenderer(mMainWindow, mMainRenderer);
	ImGui_ImplSDLRenderer3_Init(mMainRenderer);

	resetMainWindow();
}

BasicVideoSpec::~BasicVideoSpec() noexcept {
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

bool BasicVideoSpec::setViewportResolution(const s32 texture_W, const s32 texture_H) {
	mMainTexture = SDL_CreateTexture(
		mMainRenderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		texture_W, texture_H
	);

	if (!mMainTexture) {
		showErrorBox("Failed to create Viewport texture!");
	} else {
		SDL_SetTextureAlphaMod(mMainTexture, SDL_ALPHA_OPAQUE);
		SDL_SetTextureScaleMode(mMainTexture, SDL_SCALEMODE_NEAREST);
	}

	return mMainTexture ? true : false;
}

void BasicVideoSpec::setMainWindowTitle(const std::string& name) {
	const std::string windowTitle{ sAppName + name };
	SDL_SetWindowTitle(mMainWindow, windowTitle.c_str());
}

void BasicVideoSpec::setWindowTitle(SDL_Window* window, const std::string& name) {
	SDL_SetWindowTitle(window, name.c_str());
}

void BasicVideoSpec::showErrorBox(const char* const title) noexcept {
	setErrorState(true);
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title,
		SDL_GetError(), nullptr
	);
}

void BasicVideoSpec::raiseMainWindow() {
	SDL_RaiseWindow(mMainWindow);
}

void BasicVideoSpec::raiseWindow(SDL_Window* window) {
	SDL_RaiseWindow(window);
}

void BasicVideoSpec::resetMainWindow(const s32 window_W, const s32 window_H) {
	SDL_SetWindowMinimumSize(mMainWindow, window_W, window_H);
	SDL_SetWindowSize(mMainWindow, window_W, window_H);

	const std::string windowTitle{ sAppName + "Waiting for file..."s };
	SDL_SetWindowTitle(mMainWindow, windowTitle.c_str());
	SDL_ShowWindow(mMainWindow);
	SDL_SyncWindow(mMainWindow);

	mMainTexture.reset();
	renderPresent();
}

void BasicVideoSpec::setWindowSize(SDL_Window* window, const s32 window_W, const s32 window_H) {
	SDL_SetWindowSize(window, window_W, window_H);
	SDL_SyncWindow(window);
}

u32* BasicVideoSpec::lockTexture() {
	void* pixel_ptr{};
	s32 pixel_pitch{};
	SDL_LockTexture(
		mMainTexture, nullptr,
		&pixel_ptr, &pixel_pitch
	);
	return static_cast<u32*>(pixel_ptr);
}
void BasicVideoSpec::unlockTexture() {
	SDL_UnlockTexture(mMainTexture);
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

void BasicVideoSpec::setViewportOpacity(const u32 alpha) {
	SDL_SetTextureAlphaMod(mMainTexture, static_cast<Uint8>(alpha));
}

void BasicVideoSpec::setAspectRatio(
	const s32 texture_W,
	const s32 texture_H,
	const s32 padding_S
) {
	const auto padding_A{ std::abs(padding_S) };

	mOuterFramePad = padding_A;
	enableScanLine = padding_A == padding_S;

	mInnerFrame = {
		static_cast<f32>(padding_A),
		static_cast<f32>(padding_A),
		static_cast<f32>(texture_W),
		static_cast<f32>(texture_H)
	};

	mOuterFrame.w = texture_W + 2.0f * mOuterFramePad;
	mOuterFrame.h = texture_H + 2.0f * mOuterFramePad;

	SDL_SetRenderLogicalPresentation(
		mMainRenderer,
		static_cast<s32>(mOuterFrame.w),
		static_cast<s32>(mOuterFrame.h),
		SDL_LOGICAL_PRESENTATION_INTEGER_SCALE
	);

	multiplyWindowDimensions();
}

void BasicVideoSpec::multiplyWindowDimensions() {
	const auto desired_W{ static_cast<s32>(mOuterFrame.w) };
	const auto desired_H{ static_cast<s32>(mOuterFrame.h) };

	SDL_SetWindowMinimumSize(mMainWindow, desired_W, desired_H);
	setWindowSize(mMainWindow, desired_W * mScaleMultiplier, desired_H * mScaleMultiplier);
	renderPresent();
}

void BasicVideoSpec::changeScaleMultiplier(const s32 delta) {
	mScaleMultiplier = std::clamp(mScaleMultiplier + delta, 1, 8);
	multiplyWindowDimensions();
}

void BasicVideoSpec::renderPresent() {
	SDL_SetRenderDrawColor(mMainRenderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
	SDL_RenderClear(mMainRenderer);

	if (mMainTexture) {
		SDL_SetRenderDrawColor(
			mMainRenderer,
			static_cast<u8>(mOuterFrameColor[enableBuzzGlow] >> 16),
			static_cast<u8>(mOuterFrameColor[enableBuzzGlow] >>  8),
			static_cast<u8>(mOuterFrameColor[enableBuzzGlow]), SDL_ALPHA_OPAQUE
		);
		SDL_RenderFillRect(mMainRenderer, &mOuterFrame);

		SDL_SetRenderDrawColor(mMainRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(mMainRenderer, &mInnerFrame);

		SDL_RenderTexture(mMainRenderer, mMainTexture, nullptr, &mInnerFrame);

		if (enableScanLine) {
			SDL_SetRenderDrawBlendMode(mMainRenderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(mMainRenderer, 0, 0, 0, 0x20);

			const auto drawLimit{ static_cast<s32>(mOuterFrame.h) };
			for (auto y{ 0 }; y < drawLimit; y += mOuterFramePad) {
				SDL_RenderLine(
					mMainRenderer,
					mOuterFrame.x, static_cast<f32>(y),
					mOuterFrame.w, static_cast<f32>(y)
				);
			}
		}
	}

	s32 window_W, window_H;
	SDL_GetWindowSize(mMainWindow, &window_W, &window_H);

	SDL_Unique guiTexture{ SDL_CreateTexture(
		mMainRenderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_TARGET, window_W, window_H
	) };

	if (!guiTexture) {
		showErrorBox("Failed to create GUI texture!");
	} else {
		SDL_SetRenderTarget(mMainRenderer, guiTexture);
		SDL_SetRenderDrawColor(mMainRenderer, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
		SDL_RenderClear(mMainRenderer);

		#pragma region IMGUI CODE CALL/PASSTHROUGH
			ImGui_ImplSDLRenderer3_NewFrame();
			ImGui_ImplSDL3_NewFrame();
			ImGui::NewFrame();

			static bool show_demo_window{ true };
			if (show_demo_window) {
				ImGui::ShowDemoWindow(&show_demo_window);
			}

			ImGui::Render();
			ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mMainRenderer);
		#pragma endregion

		SDL_SetRenderTarget(mMainRenderer, nullptr);
		SDL_RenderTexture(mMainRenderer, guiTexture, nullptr, nullptr);
	}

	SDL_RenderPresent(mMainRenderer);
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
