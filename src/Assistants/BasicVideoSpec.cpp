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

	createMainWindow(0, 0);
	if (getErrorState()) { return; }

	createMainRenderer();
	if (getErrorState()) { return; }

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	ImGui_ImplSDL3_InitForSDLRenderer(mMainWindow, mMainRenderer);
	ImGui_ImplSDLRenderer3_Init(mMainRenderer);

	resetWindow();
}

BasicVideoSpec::~BasicVideoSpec() noexcept {
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void BasicVideoSpec::createMainWindow(const s32 window_W, const s32 window_H) {
	mMainWindow = SDL_CreateWindow(nullptr, window_W, window_H, 0);

	if (!mMainWindow) {
		showErrorBox("Failed to create SDL_Window");
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
}

void BasicVideoSpec::createMainRenderer() {
	mMainRenderer = SDL_CreateRenderer(mMainWindow, nullptr);
	
	if (!mMainRenderer) {
		showErrorBox("Failed to create SDL_Renderer");
	}
}

void BasicVideoSpec::createMainTexture(s32 texture_W, s32 texture_H) {
	texture_W = std::max<s32>(std::abs(texture_W), 1);
	texture_H = std::max<s32>(std::abs(texture_H), 1);

	mMainTexture = SDL_CreateTexture(
		mMainRenderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		texture_W, texture_H
	);

	if (!mMainTexture) {
		showErrorBox("Failed to create SDL_Texture");
	} else {
		SDL_SetTextureScaleMode(mMainTexture, SDL_SCALEMODE_NEAREST);
		mTexPixelPitch = texture_W * 4;
	}
}

bool BasicVideoSpec::updateMainTexture(s32 texture_W, s32 texture_H) {
	setTextureAlpha(0xFF);
	createMainTexture(texture_W, texture_H);
	return mMainTexture ? false : true;
}

void BasicVideoSpec::changeTitle(const std::string& name) {
	std::string windowTitle{ "CubeChip :: " + name };
	SDL_SetWindowTitle(mMainWindow, windowTitle.c_str());
}

void BasicVideoSpec::showErrorBox(const char* const title) noexcept {
	setErrorState(true);
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title,
		SDL_GetError(), nullptr
	);
}

void BasicVideoSpec::raiseWindow() {
	SDL_RaiseWindow(mMainWindow);
}

void BasicVideoSpec::resetWindow() {
	SDL_SetWindowSize(mMainWindow, 640, 480);
	changeTitle("Waiting for file...");
	mMainTexture.reset();
	renderPresent();
}

u32* BasicVideoSpec::lockTexture() {
	void* pixel_ptr{};
	SDL_LockTexture(
		mMainTexture,
		nullptr, &pixel_ptr,
		&mTexPixelPitch
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

void BasicVideoSpec::setTextureAlpha(const u32 alpha) {
	SDL_SetTextureAlphaMod(mMainTexture, static_cast<u8>(alpha));
}

void BasicVideoSpec::setAspectRatio(
	const s32 texture_W,
	const s32 texture_H,
	const s32 padding_S
) {
	const auto padding_A{ std::abs(padding_S) };

	mOuterFrameWidth = padding_A;
	enableScanLine = padding_A == padding_S;

	mOuterFrame = {
		static_cast<f32>(padding_A),
		static_cast<f32>(padding_A),
		static_cast<f32>(texture_W),
		static_cast<f32>(texture_H)
	};

	mInnerFrame.w = texture_W + 2.0f * mOuterFrameWidth;
	mInnerFrame.h = texture_H + 2.0f * mOuterFrameWidth;

	multiplyWindowDimensions();

	SDL_SetRenderLogicalPresentation(
		mMainRenderer,
		texture_W + mOuterFrameWidth * 2,
		texture_H + mOuterFrameWidth * 2,
		SDL_LOGICAL_PRESENTATION_INTEGER_SCALE
	);
}

void BasicVideoSpec::multiplyWindowDimensions() {
	const auto desired_W{ static_cast<s32>(mInnerFrame.w) };
	const auto desired_H{ static_cast<s32>(mInnerFrame.h) };

	SDL_SetWindowMinimumSize(mMainWindow, desired_W, desired_H);
	SDL_SetWindowSize(mMainWindow, desired_W * mFrameScaleMulti, desired_H * mFrameScaleMulti);
	SDL_SyncWindow(mMainWindow);
}

void BasicVideoSpec::changeFrameMultiplier(const s32 delta) {
	mFrameScaleMulti = std::clamp(mFrameScaleMulti + delta, 1, 8);
	multiplyWindowDimensions();
}

void BasicVideoSpec::renderPresent() {
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	SDL_SetRenderDrawColor(mMainRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(mMainRenderer);

	static bool show_demo_window{ true };

	if (show_demo_window) {
		ImGui::ShowDemoWindow(&show_demo_window);
	}

	if (mMainTexture) {
		SDL_SetRenderDrawColor(
			mMainRenderer,
			static_cast<u8>(mOuterFrameColor[enableBuzzGlow] >> 16),
			static_cast<u8>(mOuterFrameColor[enableBuzzGlow] >>  8),
			static_cast<u8>(mOuterFrameColor[enableBuzzGlow]), SDL_ALPHA_OPAQUE
		);
		SDL_RenderFillRect(mMainRenderer, &mInnerFrame);

		SDL_SetRenderDrawColor(
			mMainRenderer,
			static_cast<u8>(mInnerFrameColor >> 16),
			static_cast<u8>(mInnerFrameColor >>  8),
			static_cast<u8>(mInnerFrameColor), SDL_ALPHA_OPAQUE
		);
		SDL_RenderFillRect(mMainRenderer, &mOuterFrame);

		SDL_SetTextureBlendMode(mMainTexture, SDL_BLENDMODE_BLEND);
		SDL_RenderTexture(mMainRenderer, mMainTexture, nullptr, &mOuterFrame);

		if (enableScanLine) {
			SDL_SetRenderDrawBlendMode(mMainRenderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(mMainRenderer, 0, 0, 0, 32);

			const auto drawLimit{ static_cast<s32>(mInnerFrame.h) };
			for (auto y{ 0 }; y < drawLimit; y += mOuterFrameWidth) {
				SDL_RenderLine(
					mMainRenderer,
					mInnerFrame.x, static_cast<f32>(y),
					mInnerFrame.w, static_cast<f32>(y)
				);
			}
		}
	} else {
		SDL_RenderTexture(mMainRenderer, nullptr, nullptr, nullptr);
	}

	auto window_W{ 0 }, window_H{ 0 };
	SDL_GetWindowSize(mMainWindow, &window_W, &window_H);

	SDL_Unique imguiTexture{
		SDL_CreateTexture(
			mMainRenderer, SDL_PIXELFORMAT_RGBA8888,
			SDL_TEXTUREACCESS_TARGET, window_W, window_H
		)
	};

	SDL_SetRenderTarget(mMainRenderer, imguiTexture);
	SDL_RenderClear(mMainRenderer);

	ImGui::Render();
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mMainRenderer);

	SDL_SetRenderTarget(mMainRenderer, nullptr);
	SDL_RenderTexture(mMainRenderer, imguiTexture, nullptr, nullptr);
	SDL_RenderPresent(mMainRenderer);
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
