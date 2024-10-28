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

	mMainWindow = SDL_CreateWindow(sAppName, 0, 0, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
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
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

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


void BasicVideoSpec::setMainWindowTitle(const std::string& name) {
	const std::string windowTitle{ sAppName + " :: "s + name};
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
	mMainTexture = SDL_CreateTexture(
		mMainRenderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		texture_W, texture_H
	);

	if (!mMainTexture) {
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

	mOuterFramePad = padding_A;
	enableScanLine = padding_S && padding_A == padding_S;

	mInnerFrame = {
		static_cast<f32>(padding_A),
		static_cast<f32>(padding_A),
		static_cast<f32>(texture_W * upscale_M),
		static_cast<f32>(texture_H * upscale_M)
	};

	const auto window_W{ texture_W * upscale_M + 2 * mOuterFramePad };
	const auto window_H{ texture_H * upscale_M + 2 * mOuterFramePad };

	mOuterFrame.w = static_cast<f32>(window_W);
	mOuterFrame.h = static_cast<f32>(window_H);

	SDL_SetWindowMinimumSize(mMainWindow, window_W, window_H);

	return setViewportDimensions(texture_W, texture_H);
}

void BasicVideoSpec::drawViewportTexture(SDL_Texture* viewportTexture) {
	SDL_SetRenderTarget(mMainRenderer, viewportTexture);

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

	SDL_SetRenderTarget(mMainRenderer, nullptr);
}

void BasicVideoSpec::renderPresent() {
	SDL_Unique viewportTexture{ SDL_CreateTexture(
		mMainRenderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_TARGET,
		static_cast<s32>(mOuterFrame.w),
		static_cast<s32>(mOuterFrame.h)
	) };

	if (!viewportTexture) {
		showErrorBox("Failed to create GUI texture!");
		return;
	}
	
	drawViewportTexture(viewportTexture);

	#pragma region IMGUI LOGIC
		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Open...")) {

				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		const auto viewportResolution{ ImVec2{
			ImGui::GetIO().DisplaySize.x,
			ImGui::GetIO().DisplaySize.y - ImGui::GetFrameHeight()
		} };

		ImGui::SetNextWindowPos(ImVec2{ 0, ImGui::GetFrameHeight() });
		ImGui::SetNextWindowSize(viewportResolution);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });

		ImGui::Begin("ViewportFrame", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs
		);
		ImGui::Image(
			reinterpret_cast<ImTextureID>(viewportTexture.get()),
			viewportResolution
		);
		ImGui::End();
		ImGui::PopStyleVar();

		static bool show_demo_window{ true };
		if (show_demo_window) {
			ImGui::ShowDemoWindow(&show_demo_window);
		}

		ImGui::Render();
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mMainRenderer);
	#pragma endregion


	SDL_RenderPresent(mMainRenderer);
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
