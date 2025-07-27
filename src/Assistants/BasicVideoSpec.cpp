/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>
#include <limits>
#include <stdexcept>
#include <algorithm>

#ifdef _WIN32
	#define NOMINMAX
	#pragma warning(push)
	#pragma warning(disable : 5039)
	#include <dwmapi.h>
	#pragma comment(lib, "Dwmapi")
	#pragma warning(pop)
#endif

#include "FrontendInterface.hpp"
#include "BasicVideoSpec.hpp"
#include "ColorOps.hpp"

#include <SDL3/SDL.h>

/*==================================================================*/

auto BasicVideoSpec::Rect::frect() const noexcept -> SDL_FRect {
	return SDL_FRect{ 0.0f, 0.0f, f32(W), f32(H) };
}

auto BasicVideoSpec::Viewport::frect() const noexcept -> SDL_FRect {
	return SDL_FRect{ f32(ppad), f32(ppad), f32(rect.W * mult), f32(rect.H * mult) };
}

/*==================================================================*/
	#pragma region BasicVideoSpec Singleton Class

BasicVideoSpec::BasicVideoSpec(const Settings& settings) noexcept {
	mSuccessful = SDL_InitSubSystem(SDL_INIT_VIDEO);
	if (!mSuccessful) {
		showErrorBox("Failed to init Video Subsystem!");
		return;
	}

	mViewportScaleMode = settings.viewport.filtering < 0 ? 0 : settings.viewport.filtering % 3;

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

	if (SDL_Unique<SDL_Window> dummy{ SDL_CreateWindow(
		nullptr, 64, 64, SDL_WINDOW_UTILITY | SDL_WINDOW_HIDDEN
	) }) {
		#ifndef __APPLE__
		constexpr auto away{ std::numeric_limits<int>::min() };
		SDL_SetWindowPosition(dummy, away, away);
		#endif
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

	FrontendInterface::Initialize(mMainWindow, mMainRenderer);

	resetMainWindow();
}

BasicVideoSpec::~BasicVideoSpec() noexcept {
	FrontendInterface::Shutdown();

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

auto BasicVideoSpec::getTextureSizeRect(SDL_Texture* texture) const noexcept -> BasicVideoSpec::Rect {
	f32 w, h;
	SDL_GetTextureSize(texture, &w, &h);
	return Rect{
		static_cast<s32>(w),
		static_cast<s32>(h)
	};
}

auto BasicVideoSpec::exportSettings() const noexcept -> Settings {
	Settings out;

	if (SDL_GetWindowFlags(mMainWindow) & SDL_WINDOW_MAXIMIZED) {
		SDL_RestoreWindow(mMainWindow);
		SDL_SyncWindow(mMainWindow);
	}

	SDL_GetWindowPosition(mMainWindow, &out.window.x, &out.window.y);
	SDL_GetWindowSize(mMainWindow, &out.window.w, &out.window.h);
	out.viewport.filtering = mViewportScaleMode;
	out.viewport.int_scale = mIntegerScaling;
	out.viewport.scanlines = mUsingScanlines;
	out.first_run = false;

	return out;
}

void BasicVideoSpec::setMainWindowTitle(const Str& title, const Str& desc) {
	SDL_SetWindowTitle(mMainWindow, (desc.empty() ? title : title + " :: " + desc).c_str());
}

bool BasicVideoSpec::isMainWindowID(u32 id) const noexcept {
	return id == SDL_GetWindowID(mMainWindow);
}

void BasicVideoSpec::showErrorBox(const char* const title) noexcept {
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title,
		SDL_GetError(), nullptr
	);
}

auto BasicVideoSpec::computeOverlapArea(const SDL_Rect& a, const SDL_Rect& b) noexcept {
	auto xOverlap{ std::max(0, std::min(a.x + a.w, b.x + b.w) - std::max(a.x, b.x)) };
	auto yOverlap{ std::max(0, std::min(a.y + a.h, b.y + b.h) - std::max(a.y, b.y)) };
	return xOverlap * yOverlap;
}

auto BasicVideoSpec::squaredDistance(s32 x1, s32 y1, s32 x2, s32 y2) noexcept {
	s64 dx{ x1 - x2 };
	s64 dy{ y1 - y2 };
	return dx * dx + dy * dy;
}

void BasicVideoSpec::normalizeRectToDisplay(SDL_Rect& rect, SDL_Rect& deco, bool first_run) noexcept {
	auto numDisplays{  0 }; // count of displays SDL found
	auto bestDisplay{ -1 }; // index of display our window will use
	bool rectOverlap{};
	
	// 1: fetch all eligible display IDs
	SDL_Unique<SDL_DisplayID> displays{ SDL_GetDisplays(&numDisplays) };
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

	mCurViewport = { Settings::defaults.w, Settings::defaults.h, 1, 0 };
	mViewportRotation = 0;

	mSystemTexture.reset();
	mWindowTexture.reset();
}

void BasicVideoSpec::setViewportAlpha(u32 alpha) noexcept {
	mTextureAlpha.store(u8(alpha), mo::release);
}

void BasicVideoSpec::setViewportSizes(s32 W, s32 H, s32 mult, s32 ppad) noexcept {
	mNewViewport.store(Viewport::pack(W, H, mult, ppad), mo::release);
}

auto BasicVideoSpec::getViewportSizes() const noexcept -> BasicVideoSpec::Viewport {
	return Viewport::unpack(mNewViewport.load(mo::acquire));
}

void BasicVideoSpec::setViewportScaleMode(s32 mode) noexcept {
	switch (mode) {
		case SDL_SCALEMODE_NEAREST:
		case SDL_SCALEMODE_LINEAR:
		case SDL_SCALEMODE_PIXELART:
			SDL_SetTextureScaleMode(mWindowTexture,
				static_cast<SDL_ScaleMode>(mode));
			mViewportScaleMode = mode;
			[[fallthrough]];
		default: return;
	}
}

void BasicVideoSpec::cycleViewportScaleMode() noexcept {
	switch (mViewportScaleMode) {
		case SDL_SCALEMODE_NEAREST:
			setViewportScaleMode(SDL_SCALEMODE_LINEAR);
			break;
		case SDL_SCALEMODE_LINEAR:
			setViewportScaleMode(SDL_SCALEMODE_PIXELART);
			break;
		default:
			setViewportScaleMode(SDL_SCALEMODE_NEAREST);
			break;
	}
}

void BasicVideoSpec::setBorderColor(u32 color) noexcept {
	mOutlineColor.store(color, mo::release);
}

void BasicVideoSpec::scaleInterface(const void* data, size_type size) {
	FrontendInterface::UpdateFontScale(data, s32(size), SDL_GetWindowDisplayScale(mMainWindow));
}

void BasicVideoSpec::processInterfaceEvent(SDL_Event* event) const noexcept {
	FrontendInterface::ProcessEvent(event);
}

/*==================================================================*/

void BasicVideoSpec::prepareWindowTexture() {
	const auto outerRect{ mCurViewport.padded() };

	if (getTextureSizeRect(mWindowTexture) != outerRect) {
		mSuccessful = mWindowTexture = SDL_CreateTexture(
			mMainRenderer,
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_TARGET,
			outerRect.W, outerRect.H
		);

		if (!mSuccessful) {
			showErrorBox("Failed to create Window texture!");
		} else {
			SDL_SetTextureScaleMode(mWindowTexture, static_cast<SDL_ScaleMode>(mViewportScaleMode));
			SDL_SetRenderTarget(mMainRenderer, mWindowTexture);
			SDL_SetRenderDrawColor(mMainRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
			SDL_RenderClear(mMainRenderer);
		}
	}
}

void BasicVideoSpec::prepareSystemTexture() {
	if (!mWindowTexture) { return; }

	if (getTextureSizeRect(mSystemTexture) != mCurViewport.rect) {

		mSuccessful = mSystemTexture = SDL_CreateTexture(
			mMainRenderer,
			SDL_PIXELFORMAT_RGBA8888,
			SDL_TEXTUREACCESS_STREAMING,
			mCurViewport.rect.W, mCurViewport.rect.H
		);

		if (!mSuccessful) {
			showErrorBox("Failed to create System texture!");
		} else {
			SDL_SetTextureScaleMode(mSystemTexture, static_cast<SDL_ScaleMode>(mViewportScaleMode));
			SDL_SetTextureAlphaMod(mSystemTexture, mTextureAlpha.load(mo::acquire));
		}
	}
}

void BasicVideoSpec::renderViewport() {
	if (!mWindowTexture && !mSystemTexture)
		[[unlikely]] { return; }

	if (mWindowTexture) {
		SDL_SetRenderTarget(mMainRenderer, mWindowTexture);

		const RGBA Color{ mOutlineColor.load(mo::acquire) };
		SDL_SetRenderDrawColor(mMainRenderer, Color.R, Color.G, Color.B, SDL_ALPHA_OPAQUE);
		const auto outerFRect{ mCurViewport.padded().frect() };
		SDL_RenderFillRect(mMainRenderer, &outerFRect);
	}

	if (mSystemTexture) {
		SDL_SetRenderDrawColor(mMainRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		const auto innerFRect{ mCurViewport.frect() };
		SDL_RenderFillRect(mMainRenderer, &innerFRect);

		{
			void* pixels{}; s32 pitch;

			SDL_LockTexture(mSystemTexture, nullptr, &pixels, &pitch);
			displayBuffer.read(static_cast<u32*>(pixels), mCurViewport.rect);
			SDL_UnlockTexture(mSystemTexture);
		}

		SDL_RenderTexture(mMainRenderer, mSystemTexture, nullptr, &innerFRect);

		if (mUsingScanlines && mCurViewport.ppad >= 2) {
			SDL_SetRenderDrawBlendMode(mMainRenderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(mMainRenderer, 0, 0, 0, 0x20);

			const auto outerFRect{ mCurViewport.padded().frect() };
			const auto drawLimit{ static_cast<s32>(outerFRect.h) };
			for (auto y{ 0 }; y < drawLimit; y += mCurViewport.ppad) {
				SDL_RenderLine(mMainRenderer,
					outerFRect.x, static_cast<f32>(y),
					outerFRect.w, static_cast<f32>(y));
			}
		}
	}

	SDL_SetRenderTarget(mMainRenderer, nullptr);
}

void BasicVideoSpec::renderPresent(bool core, const char* overlay_data) {
	mCurViewport = getViewportSizes();

	if (core) { prepareWindowTexture(); }
	if (core) { prepareSystemTexture(); }

	renderViewport();

	FrontendInterface::NewFrame();

	const auto outerRect{ mCurViewport.rotate_if(mViewportRotation & 1).padded() };
	const auto frameHeight{ static_cast<s32>(FrontendInterface::GetFrameHeight()) };

	SDL_SetWindowMinimumSize(mMainWindow, outerRect.W, outerRect.H + frameHeight);

	FrontendInterface::PrepareViewport(
		mSuccessful && mWindowTexture, mIntegerScaling,
		outerRect.W, outerRect.H, mViewportRotation,
		overlay_data, mWindowTexture
	);
	FrontendInterface::PrepareGeneralUI();
	FrontendInterface::RenderFrame(mMainRenderer);

	SDL_RenderPresent(mMainRenderer);
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

BasicVideoSpec::Settings::Window::operator SDL_Rect() const noexcept {
	return SDL_Rect{ x, y, w, h };
}
