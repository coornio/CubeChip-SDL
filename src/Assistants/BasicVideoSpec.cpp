/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>
#include <limits>
#include <stdexcept>
#include <algorithm>

#include "../_imgui/imgui.h"
#include "../_imgui/imgui_impl_sdl3.h"
#include "../_imgui/imgui_impl_sdlrenderer3.h"

#include <SDL3/SDL_platform_defines.h>

#ifdef SDL_PLATFORM_WIN32
	#define NOMINMAX
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
	mSuccessful = SDL_InitSubSystem(SDL_INIT_VIDEO);
	if (!mSuccessful) {
		showErrorBox("Failed to init SDL video!");
		return;
	}

	mSuccessful = mMainWindow = SDL_CreateWindow(sAppName, 0, 0, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
	if (!mSuccessful) {
		showErrorBox("Failed to create main window!");
		return;
	}
	#if defined(SDL_PLATFORM_WIN32) && !defined(OLD_WINDOWS_SDK)
	else {
		const auto windowHandle{ SDL_GetPointerProperty(
			SDL_GetWindowProperties(mMainWindow),
			SDL_PROP_WINDOW_WIN32_HWND_POINTER,
			nullptr
		) };

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

	mSuccessful = mMainRenderer = SDL_CreateRenderer(mMainWindow, nullptr);
	if (!mSuccessful) {
		showErrorBox("Failed to create Main renderer!");
		return;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	//updateInterfacePixelScaling();

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

void BasicVideoSpec::setMainWindowTitle(const Str& name) {
	const auto windowTitle{ (sAppName ? sAppName : "") + " :: "s + name};
	SDL_SetWindowTitle(mMainWindow, windowTitle.c_str());
}

void BasicVideoSpec::setWindowTitle(SDL_Window* window, const Str& name) {
	SDL_SetWindowTitle(window, name.c_str());
}

void BasicVideoSpec::showErrorBox(const char* const title) noexcept {
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
	SDL_SetWindowSize(mMainWindow, window_W, window_H);

	SDL_ShowWindow(mMainWindow);
	SDL_SyncWindow(mMainWindow);

	setMainWindowTitle("Waiting for file..."s);
	setViewportDimensions(window_W, window_H, 1, 0);
	mMainTexture.reset();
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

bool BasicVideoSpec::setViewportDimensions(
	const s32 texture_W, const s32 texture_H
) {
	mSuccessful = mMainTexture = SDL_CreateTexture(
		mMainRenderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		texture_W, texture_H
	);

	if (!mSuccessful) {
		showErrorBox("Failed to create Viewport texture!");
	} else {
		SDL_SetTextureScaleMode(mMainTexture, SDL_SCALEMODE_NEAREST);
	}

	return mMainTexture ? true : false;
}

bool BasicVideoSpec::setViewportDimensions(
	const s32 texture_W, const s32 texture_H,
	const s32 upscale_M, const s32 padding_S
) {
	const auto padding_A{ std::abs(padding_S) };

	enableScanLine = padding_S > 0;

	mInnerFrame = {
		static_cast<f32>(padding_A),
		static_cast<f32>(padding_A),
		static_cast<f32>(texture_W * upscale_M),
		static_cast<f32>(texture_H * upscale_M)
	};

	const auto window_W{ texture_W * upscale_M + 2 * padding_A };
	const auto window_H{ texture_H * upscale_M + 2 * padding_A };

	mOuterFrame.w = static_cast<f32>(window_W);
	mOuterFrame.h = static_cast<f32>(window_H);

	return setViewportDimensions(texture_W, texture_H);
}

void BasicVideoSpec::processInterfaceEvent(SDL_Event* event) const noexcept {
	ImGui_ImplSDL3_ProcessEvent(event);
}

void BasicVideoSpec::updateInterfacePixelScaling(const void* fontData, const s32 fontSize, const f32 newScale) {
	static auto currentScale{ 0.0f };

	if (newScale < 1.0f) { return; }
	if (std::fabs(currentScale - newScale) > Epsilon::f32) {
		currentScale = newScale;
		auto& io{ ImGui::GetIO() };

		if (fontData && fontSize) {
			io.Fonts->AddFontFromMemoryCompressedTTF(
				fontData, fontSize, newScale * 17.0f
			);
		} else {
			ImFontConfig fontConfig;
			fontConfig.SizePixels = 16.0f * newScale;

			io.Fonts->Clear();
			io.Fonts->AddFontDefault(&fontConfig);
		}
		ImGui::GetStyle().ScaleAllSizes(newScale);
	}
}

void BasicVideoSpec::drawViewportTexture(SDL_Texture* viewportTexture) {
	SDL_SetRenderDrawColor(mMainRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(mMainRenderer);

	if (mMainTexture) {
		SDL_SetRenderTarget(mMainRenderer, viewportTexture);

		SDL_SetRenderDrawColor(
			mMainRenderer,
			static_cast<u8>(mOuterFrameColor[enableBuzzGlow] >> 16),
			static_cast<u8>(mOuterFrameColor[enableBuzzGlow] >> 8),
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
			const auto increment{ static_cast<s32>(mInnerFrame.y) };
			for (auto y{ 0 }; y < drawLimit; y += increment) {
				SDL_RenderLine(
					mMainRenderer,
					mOuterFrame.x, static_cast<f32>(y),
					mOuterFrame.w, static_cast<f32>(y)
				);
			}
		}
		SDL_SetRenderTarget(mMainRenderer, nullptr);
	}
}

void BasicVideoSpec::renderPresent() {
	SDL_Unique viewportTexture{ SDL_CreateTexture(
		mMainRenderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_TARGET,
		static_cast<s32>(mOuterFrame.w),
		static_cast<s32>(mOuterFrame.h)
	) };

	mSuccessful = viewportTexture;
	if (!mSuccessful) {
		showErrorBox("Failed to create GUI texture!");
		return;
	} else {
		SDL_SetTextureScaleMode(viewportTexture, SDL_SCALEMODE_NEAREST);
	}
	
	drawViewportTexture(viewportTexture);

	#pragma region IMGUI LOGIC
		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		SDL_SetWindowMinimumSize(
			mMainWindow,
			static_cast<s32>(mOuterFrame.w),
			static_cast<s32>(mOuterFrame.h + ImGui::GetFrameHeight())
		);

		const auto viewportFrameDimensions{ ImVec2{
			ImGui::GetIO().DisplaySize.x,
			ImGui::GetIO().DisplaySize.y - ImGui::GetFrameHeight()
		} };

		ImGui::SetNextWindowSize(viewportFrameDimensions);
		ImGui::SetNextWindowPos(ImVec2{ 0, ImGui::GetFrameHeight() });
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("ViewportFrame", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoBringToFrontOnFocus
		);
		ImGui::PopStyleVar();

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Open...")) {

				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
		
		const auto aspectRatio{ std::min(
			viewportFrameDimensions.x / mOuterFrame.w,
			viewportFrameDimensions.y / mOuterFrame.h
		) };

		const auto viewportDimensions{ ImVec2{
			mOuterFrame.w * std::max(std::floor(aspectRatio), 1.0f),
			mOuterFrame.h * std::max(std::floor(aspectRatio), 1.0f)
		} };

		const auto viewportOffsets{ ImVec2{
			(viewportFrameDimensions.x - viewportDimensions.x) / 2.0f,
			(viewportFrameDimensions.y - viewportDimensions.y) / 2.0f
		} };

		if (viewportOffsets.x > 0.0f) { ImGui::SetCursorPosX(ImGui::GetCursorPosX() + viewportOffsets.x); }
		if (viewportOffsets.y > 0.0f) { ImGui::SetCursorPosY(ImGui::GetCursorPosY() + viewportOffsets.y); }

		ImGui::Image(reinterpret_cast<ImTextureID>(viewportTexture.get()), viewportDimensions);
		ImGui::End();

		//static bool show_demo_window{ true };
		//if (show_demo_window) {
		//	ImGui::ShowDemoWindow(&show_demo_window);
		//}

		ImGui::Render();
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mMainRenderer);
	#pragma endregion


	SDL_RenderPresent(mMainRenderer);
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
