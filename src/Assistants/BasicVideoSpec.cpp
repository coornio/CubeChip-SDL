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

	ImGui_ImplSDL3_InitForSDLRenderer(mMainWindow.get(), mMainRenderer.get());
	ImGui_ImplSDLRenderer3_Init(mMainRenderer.get());

	resetWindow();
}

BasicVideoSpec::~BasicVideoSpec() noexcept {
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void BasicVideoSpec::createMainWindow(const s32 window_W, const s32 window_H) {
	mMainWindow.reset(SDL_CreateWindow(nullptr, window_W, window_H, 0));

	if (!mMainWindow) {
		showErrorBox("Failed to create SDL_Window");
	}
	#if defined(SDL_PLATFORM_WIN32) && !defined(OLD_WINDOWS_SDK)
	else {
		const auto windowHandle{
			SDL_GetPointerProperty(
				SDL_GetWindowProperties(mMainWindow.get()),
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
	mMainRenderer.reset(SDL_CreateRenderer(mMainWindow.get(), nullptr));
	
	if (!mMainRenderer) {
		showErrorBox("Failed to create SDL_Renderer");
	}
}

void BasicVideoSpec::createMainTexture(s32 texture_W, s32 texture_H) {
	texture_W = std::max<s32>(std::abs(texture_W), 1);
	texture_H = std::max<s32>(std::abs(texture_H), 1);

	mMainTexture.reset(SDL_CreateTexture(
		mMainRenderer.get(),
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		texture_W, texture_H
	));

	if (!mMainTexture) {
		showErrorBox("Failed to create SDL_Texture");
	} else {
		SDL_SetTextureScaleMode(mMainTexture.get(), SDL_SCALEMODE_NEAREST);
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
	SDL_SetWindowTitle(mMainWindow.get(), windowTitle.c_str());
}

void BasicVideoSpec::showErrorBox(const char* const title) noexcept {
	setErrorState(true);
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title,
		SDL_GetError(), nullptr
	);
}

void BasicVideoSpec::raiseWindow() {
	SDL_RaiseWindow(mMainWindow.get());
}

void BasicVideoSpec::resetWindow() {
	SDL_SetWindowSize(mMainWindow.get(), 640, 480);
	changeTitle("Waiting for file...");
	mMainTexture.reset();
	renderPresent();
}

u32* BasicVideoSpec::lockTexture() {
	void* pixel_ptr{};
	SDL_LockTexture(
		mMainTexture.get(),
		nullptr, &pixel_ptr,
		&mTexPixelPitch
	);
	return static_cast<u32*>(pixel_ptr);
}
void BasicVideoSpec::unlockTexture() {
	SDL_UnlockTexture(mMainTexture.get());
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
	SDL_SetTextureAlphaMod(mMainTexture.get(), static_cast<u8>(alpha));
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
		mMainRenderer.get(),
		texture_W + mOuterFrameWidth * 2,
		texture_H + mOuterFrameWidth * 2,
		SDL_LOGICAL_PRESENTATION_INTEGER_SCALE
	);
}

void BasicVideoSpec::multiplyWindowDimensions() {
	const auto desired_W{ static_cast<s32>(mInnerFrame.w) };
	const auto desired_H{ static_cast<s32>(mInnerFrame.h) };

	SDL_SetWindowMinimumSize(mMainWindow.get(), desired_W, desired_H);
	SDL_SetWindowSize(mMainWindow.get(), desired_W * mFrameScaleMulti, desired_H * mFrameScaleMulti);
	SDL_SyncWindow(mMainWindow.get());

	auto window_W{ 0 }, window_H{ 0 };
	SDL_GetWindowSize(mMainWindow.get(), &window_W, &window_H);

	auto render_W{ 0 }, render_H{ 0 };
	SDL_GetCurrentRenderOutputSize(mMainRenderer.get(), &render_W, &render_H);

		// Ensure ImGui is using the correct display size (in screen coordinates)
	ImGui::GetIO().DisplaySize = ImVec2(static_cast<f32>(window_W), static_cast<f32>(window_H));

	// Calculate framebuffer scale based on the rendering output size
	ImGui::GetIO().DisplayFramebufferScale = ImVec2(
		static_cast<f32>(render_W) / static_cast<f32>(window_W),
		static_cast<f32>(render_H) / static_cast<f32>(window_H)
	);

	// Ensure there's no additional global scaling
	ImGui::GetIO().FontGlobalScale = 1.0f;

}

void BasicVideoSpec::changeFrameMultiplier(const s32 delta) {
	mFrameScaleMulti = std::clamp(mFrameScaleMulti + delta, 1, 8);
	multiplyWindowDimensions();
}

void BasicVideoSpec::renderPresent() {
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	SDL_SetRenderDrawColor(mMainRenderer.get(), 0, 0, 0, 255);
	SDL_RenderClear(mMainRenderer.get());

	static bool show_demo_window{ true };

	if (show_demo_window) {
		ImGui::ShowDemoWindow(&show_demo_window);
	}

	if (mMainTexture) {
		SDL_SetRenderDrawColor(
			mMainRenderer.get(),
			static_cast<u8>(mOuterFrameColor[enableBuzzGlow] >> 16),
			static_cast<u8>(mOuterFrameColor[enableBuzzGlow] >>  8),
			static_cast<u8>(mOuterFrameColor[enableBuzzGlow]), SDL_ALPHA_OPAQUE
		);
		SDL_RenderFillRect(mMainRenderer.get(), &mInnerFrame);

		SDL_SetRenderDrawColor(
			mMainRenderer.get(),
			static_cast<u8>(mInnerFrameColor >> 16),
			static_cast<u8>(mInnerFrameColor >>  8),
			static_cast<u8>(mInnerFrameColor), SDL_ALPHA_OPAQUE
		);
		SDL_RenderFillRect(mMainRenderer.get(), &mOuterFrame);

		SDL_SetTextureBlendMode(mMainTexture.get(), SDL_BLENDMODE_BLEND);
		SDL_RenderTexture(mMainRenderer.get(), mMainTexture.get(), nullptr, &mOuterFrame);

		if (enableScanLine) {
			SDL_SetRenderDrawBlendMode(mMainRenderer.get(), SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(mMainRenderer.get(), 0, 0, 0, 32);

			const auto drawLimit{ static_cast<s32>(mInnerFrame.h) };
			for (auto y{ 0 }; y < drawLimit; y += mOuterFrameWidth) {
				SDL_RenderLine(
					mMainRenderer.get(),
					mInnerFrame.x, static_cast<f32>(y),
					mInnerFrame.w, static_cast<f32>(y)
				);
			}
		}
	} else {
		SDL_RenderTexture(mMainRenderer.get(), nullptr, nullptr, nullptr);
	}

	ImGui::Render();
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mMainRenderer.get());
	SDL_RenderPresent(mMainRenderer.get());
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
