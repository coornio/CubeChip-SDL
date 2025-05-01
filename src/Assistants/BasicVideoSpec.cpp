/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>
#include <limits>
#include <stdexcept>
#include <algorithm>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../Libraries/imgui/imgui.h"
#include "../Libraries/imgui/imgui_impl_sdl3.h"
#include "../Libraries/imgui/imgui_impl_sdlrenderer3.h"

#ifdef _WIN32
	#define NOMINMAX
	#pragma warning(push)
	#pragma warning(disable : 5039)
	#include <dwmapi.h>
	#pragma comment(lib, "Dwmapi")
	#pragma warning(pop)
#endif

#include "BasicVideoSpec.hpp"
#include "RGBA.hpp"

/*==================================================================*/
	#pragma region BasicVideoSpec Singleton Class

BasicVideoSpec::BasicVideoSpec(const Settings& settings) noexcept {
	mSuccessful = SDL_InitSubSystem(SDL_INIT_VIDEO);
	if (!mSuccessful) {
		showErrorBox("Failed to init SDL video!");
		return;
	}

	mViewportScaleMode = (settings.viewport.filtering < 0) ? 0 : settings.viewport.filtering % 3;

	mIntegerScaling    = settings.viewport.int_scale;
	mUsingScanlines    = settings.viewport.scanlines;

	mSuccessful = mMainWindow = SDL_CreateWindow(nullptr, 0, 0, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
	if (!mSuccessful) {
		showErrorBox("Failed to create main window!");
		return;
	}
	#if defined(_WIN32) && !defined(OLD_WINDOWS_SDK)
	else {
		const auto windowHandle{ SDL_GetPointerProperty(
			SDL_GetWindowProperties(mMainWindow),
			SDL_PROP_WINDOW_WIN32_HWND_POINTER,
			nullptr
		) };

		if (windowHandle) {
			const auto cornerMode{ DWMWCP_DONOTROUND };
			DwmSetWindowAttribute(
				static_cast<HWND>(windowHandle),
				DWMWA_WINDOW_CORNER_PREFERENCE,
				&cornerMode, sizeof(cornerMode)
			);
		}
	}
	#endif

	SDL_Rect deco{};

	if (SDL_Unique dummy{ SDL_CreateWindow(
		nullptr, 64, 64, SDL_WINDOW_UTILITY | SDL_WINDOW_HIDDEN
	) }) {
		constexpr auto away{ std::numeric_limits<int>::min() };
		SDL_SetWindowPosition(dummy, away, away);
		SDL_ShowWindow(dummy);
		SDL_SyncWindow(dummy);
		SDL_RenderPresent(mMainRenderer);
		SDL_GetWindowBordersSize(dummy, &deco.x, &deco.y, &deco.w, &deco.h);
	}

	SDL_Rect rect{
		settings.window.x, settings.window.y,
		settings.window.w, settings.window.h
	};
	normalizeRectToDisplay(rect, deco, settings.first_run);

	SDL_SetWindowPosition(mMainWindow, rect.x, rect.y);
	SDL_SetWindowSize(mMainWindow, rect.w, rect.h);

	mSuccessful = mMainRenderer = SDL_CreateRenderer(mMainWindow, nullptr);
	if (!mSuccessful) {
		showErrorBox("Failed to create Main renderer!");
		return;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO().IniFilename = nullptr;
	ImGui::GetIO().LogFilename = nullptr;
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

void BasicVideoSpec::setMainWindowTitle(const Str& title, const Str& desc) {
	SDL_SetWindowTitle(mMainWindow, (desc.empty() ? title : title + " :: " + desc).c_str());
}

void BasicVideoSpec::showErrorBox(const char* const title) noexcept {
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title,
		SDL_GetError(), nullptr
	);
}

void BasicVideoSpec::normalizeRectToDisplay(SDL_Rect& rect, SDL_Rect& deco, bool first_run) noexcept {
	auto numDisplays{  0 }; // count of displays SDL found
	auto bestDisplay{ -1 }; // index of display our window will use
	bool rectOverlap{};
	
	// 1: fetch all eligible display IDs
	SDL_Unique displays{ SDL_GetDisplays(&numDisplays) };
	if (!displays || numDisplays <= 0) [[unlikely]]
		{ rect = Settings::defaults; return; }

	// 2: fill vector with usable display bounds rects
	std::vector<SDL_Rect> displayBounds;
	displayBounds.reserve(numDisplays);

	for (auto i{ 0 }; i < numDisplays; ++i) {
		if (displays[i] == SDL_GetPrimaryDisplay()) { bestDisplay = i; }
		SDL_Rect usableBounds;
		if (SDL_GetDisplayUsableBounds(displays[i], &usableBounds))
			{ displayBounds.push_back(usableBounds); }
	}
	if (displayBounds.empty()) [[unlikely]]
		{ rect = Settings::defaults; return; }

	// 3: validate rect w/h, use fallbacks if needed
	rect.w = std::max(rect.w, Settings::defaults.w);
	rect.h = std::max(rect.h, Settings::defaults.h);

	if (!first_run) {
		// 4: find largest window/display overlap, if any
		auto bestOverlap{ 0 };
		for (auto i{ 0 }; i < static_cast<s32>(displayBounds.size()); ++i) {
			const auto overlap{ computeOverlapArea(rect, displayBounds[i]) };
			if (overlap > bestOverlap) { bestOverlap = overlap; bestDisplay = i; }
		}
	
		// 5: fall back to searching for closest display
		if (!(rectOverlap = bestOverlap != 0)) {
			auto bestDistance{ std::numeric_limits<s64>::max() };
			const auto cx{ rect.x + rect.w / 2 };
			const auto cy{ rect.y + rect.h / 2 };
	
			for (auto i{ 0 }; i < static_cast<s32>(displayBounds.size()); ++i) {
				const auto tcx{ displayBounds[i].x + displayBounds[i].w / 2 };
				const auto tcy{ displayBounds[i].y + displayBounds[i].h / 2 };
				const auto distance{ squaredDistance(cx, cy, tcx, tcy) };
				if (distance < bestDistance) { bestDistance = distance; bestDisplay = i; }
			}
		}
	}

	// 6: shrink window to best fit chosen display
	const auto& target{ displayBounds[bestDisplay] };

	const auto up{ deco.x }, lt{ deco.y };
	const auto dn{ deco.w }, rt{ deco.h };

	rect.w = std::min(rect.w, target.w - lt - rt);
	rect.h = std::min(rect.h, target.h - up - dn);

	if (!rectOverlap) { // 7a: if we didn't overlap before, center to display
		rect.x = target.x + (target.w - lt - rt - rect.w) / 2 + lt;
		rect.y = target.y + (target.h - up - dn - rect.h) / 2 + up;
	} else {            // 7b: otherwise, clamp origin to lie within display bounds
		rect.x = std::clamp(rect.x, target.x + lt, target.x + target.w - rt - rect.w);
		rect.y = std::clamp(rect.y, target.y + up, target.y + target.h - dn - rect.h);
	}
}

void BasicVideoSpec::raiseMainWindow() {
	SDL_RaiseWindow(mMainWindow);
}

void BasicVideoSpec::resetMainWindow() {
	SDL_ShowWindow(mMainWindow);
	//SDL_SyncWindow(mMainWindow);

	mViewportFrame = { mViewportFrame.rect.W, mViewportFrame.rect.H };
	mViewportRotation = 0;

	mInnerTexture.reset();
	mOuterTexture.reset();
	mTextureSize.store(nullptr, mo::release);
	mNewTextureNeeded.store(false, mo::release);
}

void BasicVideoSpec::setViewportAlpha(u32 alpha) {
	mTextureAlpha.store(static_cast<u8>(alpha), mo::release);
}

void BasicVideoSpec::setViewportSizes(s32 texture_W, s32 texture_H, s32 upscale_M, s32 padding_S) {
	if (upscale_M) { mTextureScale.store(std::abs(upscale_M), mo::release); }
	if (padding_S) { mFramePadding.store(padding_S, mo::release); }

	if (texture_W > 0 && texture_H > 0) {
		if (!mNewTextureNeeded.load(mo::acquire)) {
			const auto newSize{ std::make_shared<Rect>(texture_W, texture_H) };
			const auto oldSize{ mTextureSize.exchange(newSize, mo::acq_rel) };

			mNewTextureNeeded.store((oldSize ? *oldSize : 0) != *newSize, mo::release);
		}
	}
}

void BasicVideoSpec::setBorderColor(u32 color) noexcept {
	mOutlineColor.store(color, mo::release);
}

void BasicVideoSpec::processInterfaceEvent(SDL_Event* event) const noexcept {
	ImGui_ImplSDL3_ProcessEvent(event);
}

void BasicVideoSpec::updateInterfacePixelScaling(const void* fontData, s32 fontSize, f32 newScale) {
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

/*==================================================================*/

void BasicVideoSpec::renderViewport() {
	const auto texture{ mTextureSize.load(mo::acquire) };
	if (!texture) { return; }

	if (mNewTextureNeeded.load(mo::acquire)) {
		const auto padding{ mFramePadding.load(mo::acquire) };
		const auto scaling{ mTextureScale.load(mo::acquire) };

		mViewportFrame = { *texture, scaling, std::abs(padding) };

		const auto outerRect{ mViewportFrame.padded() };
		mSuccessful = mOuterTexture = SDL_CreateTexture(
			mMainRenderer,
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_TARGET,
			outerRect.W, outerRect.H
		);

		if (!mSuccessful) {
			showErrorBox("Failed to create GUI texture!");
			return;
		} else {
			if (mViewportScaleMode >= 0 && mViewportScaleMode <= 2) {
				SDL_SetTextureScaleMode(mOuterTexture, static_cast<SDL_ScaleMode>(mViewportScaleMode));
			} else {
				mViewportScaleMode = SDL_SCALEMODE_NEAREST;
				SDL_SetTextureScaleMode(mOuterTexture, SDL_SCALEMODE_NEAREST);
			}
			SDL_SetRenderTarget(mMainRenderer, mOuterTexture);
			SDL_SetRenderDrawColor(mMainRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
			SDL_RenderClear(mMainRenderer);
		}

		const auto textureRect{ mViewportFrame.rect };
		mSuccessful = mInnerTexture = SDL_CreateTexture(
			mMainRenderer,
			SDL_PIXELFORMAT_RGBA8888,
			SDL_TEXTUREACCESS_STREAMING,
			textureRect.W, textureRect.H
		);

		if (!mSuccessful) {
			showErrorBox("Failed to create Viewport texture!");
			return;
		} else {
			SDL_SetTextureScaleMode(mInnerTexture, SDL_SCALEMODE_NEAREST);
			SDL_SetTextureAlphaMod(mInnerTexture, mTextureAlpha.load(mo::acquire));
		}

		mNewTextureNeeded.store(false, mo::release);
	}

	{
		void* pixels{}; s32 pitch;

		SDL_LockTexture(mInnerTexture, nullptr, &pixels, &pitch);
		displayBuffer.read(static_cast<u32*>(pixels), *texture);
		SDL_UnlockTexture(mInnerTexture);
	}

	SDL_SetRenderTarget(mMainRenderer, mOuterTexture);

	const RGBA Color{ mOutlineColor.load(mo::acquire) };
	SDL_SetRenderDrawColor(mMainRenderer, Color.R, Color.G, Color.B, SDL_ALPHA_OPAQUE);
	const auto outerFRect{ mViewportFrame.padded().frect() };
	SDL_RenderFillRect(mMainRenderer, &outerFRect);

	SDL_SetRenderDrawColor(mMainRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	const auto innerFRect{ mViewportFrame.frect() };
	SDL_RenderFillRect(mMainRenderer, &innerFRect);

	SDL_RenderTexture(mMainRenderer, mInnerTexture, nullptr, &innerFRect);

	if (mUsingScanlines) {
		SDL_SetRenderDrawBlendMode(mMainRenderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(mMainRenderer, 0, 0, 0, 0x20);

		const auto drawLimit{ static_cast<s32>(outerFRect.h) };
		for (auto y{ 0 }; y < drawLimit; y += mViewportFrame.pad) {
			SDL_RenderLine(mMainRenderer,
				outerFRect.x,
				static_cast<f32>(y),
				outerFRect.w,
				static_cast<f32>(y)
			);
		}
	}

	SDL_SetRenderTarget(mMainRenderer, nullptr);
}

enum class Corner {
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight
};

namespace ImGui {
	[[maybe_unused]]
	inline auto clamp(const ImVec2& value, const ImVec2& min, const ImVec2& max) {
		return ImVec2{
			std::clamp(value.x, min.x, max.x),
			std::clamp(value.y, min.y, max.y)
		};
	}

	[[maybe_unused]]
	inline auto abs(const ImVec2& value) {
		return ImVec2{ std::abs(value.x), std::abs(value.y) };
	}

	[[maybe_unused]]
	static void writeText(
		const Str& textString,
		ImVec2 textAlign   = ImVec2{ 0.5f, 0.5f },
		ImVec4 textColor   = ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f },
		ImVec2 textPadding = ImVec2{ 6.0f, 6.0f }
	) {
		using namespace ImGui;
		const auto textPos{ (
			GetWindowSize() - CalcTextSize(textString.c_str()) - textPadding * 2
		) * textAlign + textPadding };

		PushStyleColor(ImGuiCol_Text, textColor);
		SetCursorPos(textPos);
		TextUnformatted(textString.c_str());
		PopStyleColor();
	}

	[[maybe_unused]]
	static void writeShadowedText(
		const Str& textString,
		ImVec2 textAlign   = ImVec2{ 0.5f, 0.5f },
		ImVec4 textColor   = ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f },
		ImVec2 textPadding = ImVec2{ 6.0f, 6.0f },
		ImVec2 shadowDist  = ImVec2{ 2.0f, 2.0f }
	) {
		using namespace ImGui;
		const auto textPos{ (
			GetWindowSize() - CalcTextSize(textString.c_str()) - textPadding * 2
		) * textAlign + textPadding };

		const auto shadowOffset{ shadowDist * 0.5f };
		PushStyleColor(ImGuiCol_Text, { 0.0f, 0.0f, 0.0f, 1.0f });
		SetCursorPos(textPos + shadowOffset);
		TextUnformatted(textString.c_str());
		PopStyleColor();

		PushStyleColor(ImGuiCol_Text, textColor);
		SetCursorPos(textPos - shadowOffset);
		TextUnformatted(textString.c_str());
		PopStyleColor();
	}

	static void DrawRotatedImage(void* texture, const ImVec2& dimensions, int rotation) {
		static constexpr ImVec2 TL{ 0, 0 };
		static constexpr ImVec2 TR{ 1, 0 };
		static constexpr ImVec2 BL{ 0, 1 };
		static constexpr ImVec2 BR{ 1, 1 };

		const auto pos{ ImGui::GetCursorScreenPos() };

		const ImVec2 A{ pos.x,                pos.y };
		const ImVec2 B{ pos.x + dimensions.x, pos.y };
		const ImVec2 C{ pos.x + dimensions.x, pos.y + dimensions.y };
		const ImVec2 D{ pos.x,                pos.y + dimensions.y };

		switch (rotation & 3) {
			case 0:
				ImGui::GetWindowDrawList()->AddImageQuad(
					reinterpret_cast<ImTextureID>(texture), A, B, C, D, TL, TR, BR, BL);
				break;

			case 1:
				ImGui::GetWindowDrawList()->AddImageQuad(
					reinterpret_cast<ImTextureID>(texture), A, B, C, D, BL, TL, TR, BR);
				break;

			case 2:
				ImGui::GetWindowDrawList()->AddImageQuad(
					reinterpret_cast<ImTextureID>(texture), A, B, C, D, BR, BL, TL, TR);
				break;

			case 3:
				ImGui::GetWindowDrawList()->AddImageQuad(
					reinterpret_cast<ImTextureID>(texture), A, B, C, D, TR, BR, BL, TL);
				break;
		}
		ImGui::Dummy(dimensions);
	}
}

void BasicVideoSpec::renderPresent(const char* const stats) {
	renderViewport();

	#pragma region IMGUI LOGIC
		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		const auto outerRect{ mViewportFrame.rotate_if(mViewportRotation & 1).padded() };
		const auto frameHeight{ static_cast<s32>(ImGui::GetFrameHeight()) };

		SDL_SetWindowMinimumSize(mMainWindow, outerRect.W, outerRect.H + frameHeight);

		const auto viewportFrameDimensions{ ImVec2{
			ImGui::GetIO().DisplaySize.x,
			ImGui::GetIO().DisplaySize.y - ImGui::GetFrameHeight()
		} };

		ImGui::SetNextWindowSize(viewportFrameDimensions);
		ImGui::SetNextWindowPos({ 0.0f, ImGui::GetFrameHeight() });

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
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

		if (mSuccessful && mOuterTexture) {
			const auto aspectRatio{ mIntegerScaling
				? std::floor(std::min(
					viewportFrameDimensions.x / outerRect.W,
					viewportFrameDimensions.y / outerRect.H
				))
				: std::min(
					viewportFrameDimensions.x / outerRect.W,
					viewportFrameDimensions.y / outerRect.H
				)
			};

			const auto viewportDimensions{ ImVec2{
				outerRect.W * std::max(aspectRatio, 1.0f),
				outerRect.H * std::max(aspectRatio, 1.0f)
			} };

			const auto viewportOffsets{ (viewportFrameDimensions - viewportDimensions) / 2.0f };

			if (viewportOffsets.x > 0.0f) { ImGui::SetCursorPosX(std::floor(ImGui::GetCursorPosX() + viewportOffsets.x)); }
			if (viewportOffsets.y > 0.0f) { ImGui::SetCursorPosY(std::floor(ImGui::GetCursorPosY() + viewportOffsets.y)); }

			ImGui::DrawRotatedImage(mOuterTexture, viewportDimensions, mViewportRotation);

			if (stats) { ImGui::writeShadowedText(stats, { 0.0f, 1.0f }); }
		}

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
