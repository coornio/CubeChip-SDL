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
#include "RGBA.hpp"

#include <SDL3/SDL.h>

/*==================================================================*/

auto BasicVideoSpec::Rect::frect() const noexcept -> SDL_FRect {
	return SDL_FRect{
		static_cast<f32>(0),
		static_cast<f32>(0),
		static_cast<f32>(W),
		static_cast<f32>(H)
	};
}

auto BasicVideoSpec::Viewport::frect() const noexcept -> SDL_FRect {
	return SDL_FRect{
		static_cast<f32>(pad),
		static_cast<f32>(pad),
		static_cast<f32>(rect.W * scale),
		static_cast<f32>(rect.H * scale)
	};
}

/*==================================================================*/
	#pragma region BasicVideoSpec Singleton Class

BasicVideoSpec::BasicVideoSpec(const Settings& settings) noexcept {
	mSuccessful = SDL_InitSubSystem(SDL_INIT_VIDEO);
	if (!mSuccessful) {
		showErrorBox("Failed to init Video Subsystem!");
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

	FrontendInterface::Initialize(mMainWindow, mMainRenderer);

	resetMainWindow();
}

BasicVideoSpec::~BasicVideoSpec() noexcept {
	FrontendInterface::Shutdown();

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

auto BasicVideoSpec::exportSettings() const noexcept -> BasicVideoSpec::Settings {
	Settings out;

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

void BasicVideoSpec::setViewportScaleMode(s32 mode) noexcept {
	switch (mode) {
		case SDL_SCALEMODE_NEAREST:
		case SDL_SCALEMODE_LINEAR:
			SDL_SetTextureScaleMode(mOuterTexture,
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
	FrontendInterface::UpdateFontScale(data, static_cast<s32>(size), SDL_GetWindowDisplayScale(mMainWindow));
}

void BasicVideoSpec::processInterfaceEvent(SDL_Event* event) const noexcept {
	FrontendInterface::ProcessEvent(event);
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

void BasicVideoSpec::renderPresent(const char* overlay_data) {
	renderViewport();

	FrontendInterface::NewFrame();

	const auto outerRect{ mViewportFrame.rotate_if(mViewportRotation & 1).padded() };
	const auto frameHeight{ static_cast<s32>(FrontendInterface::GetFrameHeight()) };

	SDL_SetWindowMinimumSize(mMainWindow, outerRect.W, outerRect.H + frameHeight);

	FrontendInterface::PrepareViewport(
		mSuccessful && mOuterTexture, mIntegerScaling,
		outerRect.W, outerRect.H, mViewportRotation,
		overlay_data, mOuterTexture
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
