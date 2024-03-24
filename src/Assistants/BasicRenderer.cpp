/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "BasicRenderer.hpp"

BasicRenderer::BasicRenderer(const Sint32 w, const Sint32 h)
	: window_W(w) // placeholder
	, window_H(h) // placeholder
	, emuName{ "CubeChip" }
	, emuVersion{ "[24.03.24]" }
{
	try {
		SDL_Init(SDL_INIT_VIDEO);
		createWindow();
		createRenderer();
	}
	catch (const std::exception& e) {
		showErrorBoxSDL(e.what());
		throw;
	}
}

BasicRenderer::~BasicRenderer() {
	quitTexture();
	quitRenderer();
	quitWindow();
}

void BasicRenderer::createWindow() {
	quitWindow();

	if (!(window = SDL_CreateWindow(
		windowTitle.c_str(),
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		window_W, window_H,
		SDL_WINDOW_RESIZABLE |
		SDL_WINDOW_INPUT_FOCUS |
		SDL_WINDOW_ALLOW_HIGHDPI
	))) {
		throw std::exception("SDL Error: Window");
	}
}

void BasicRenderer::createRenderer() {
	quitRenderer();

	if (!(renderer = SDL_CreateRenderer(
		window, -1,
		SDL_RENDERER_ACCELERATED /*|
		SDL_RENDERER_PRESENTVSYNC*/
		// conflicts with the current frameLimiter setup
		// don't know how to marry the two..
	))) {
		throw std::exception("SDL Error: Renderer");
	}
}

void BasicRenderer::createTexture(const Sint32 width, const Sint32 height) {
	quitTexture();

	if (!(texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		width, height
	))) {
		throw std::exception("SDL Error: Texture");
	} else {
		ppitch = width * 4;
	}
}

void BasicRenderer::changeTitle(const std::string_view name) {
	windowTitle  = emuVersion + " :: ";
	windowTitle += emuName    + " :: ";
	windowTitle += name;
	SDL_SetWindowTitle(window, windowTitle.c_str());
}

bool BasicRenderer::showErrorBoxSDL(
	std::string_view title
) {
	return SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title.data(),
		SDL_GetError(), nullptr
	);
}

bool BasicRenderer::showErrorBox(
	std::string_view message,
	std::string_view title
) {
	return SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title.data(),
		message.data(), nullptr
	);
}

void BasicRenderer::lockTexture() {
	void* pixel_ptr{ pixels };
	SDL_LockTexture(
		texture, nullptr,
		static_cast<void**>(&pixel_ptr),
		&ppitch
	);
	pixels = static_cast<Uint32*>(pixel_ptr);
}
void BasicRenderer::unlockTexture() {
	SDL_UnlockTexture(texture);
}

void BasicRenderer::setTextureAlpha(const std::size_t alpha) {
	SDL_SetTextureAlphaMod(texture, static_cast<Uint8>(alpha));
}
void BasicRenderer::setTextureBlend(const SDL_BlendMode blend) {
	SDL_SetTextureBlendMode(texture, blend);
}
void BasicRenderer::setAspectRatio(const float ratio) {
	aspect = ratio;
	// lackluster ain't it
}

void BasicRenderer::renderPresent() {
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, nullptr, nullptr);
	SDL_RenderPresent(renderer);
}

void BasicRenderer::quitTexture() {
	if (texture)  SDL_DestroyTexture(texture);
}
void BasicRenderer::quitRenderer() {
	if (renderer) SDL_DestroyRenderer(renderer);
}
void BasicRenderer::quitWindow() {
	if (window)   SDL_DestroyWindow(window);
}

void BasicRenderer::getWindowSize(const bool resize) {
	// const auto old_W{ window_W }, old_H{ window_H };
	SDL_GetWindowSize(window, &window_W, &window_H);

	if (resize) {
		window_W &= 0x0FFFFFFC;
		window_W = std::max(window_W, 640);
		window_H = static_cast<Sint32>(window_W / aspect);
		SDL_SetWindowSize(window, window_W, window_H);
		/*
		const float aspect_window =
			window_W / 1.0f / window_H;

		if (aspect > aspect_window) {
			screen.w = window_W;
			screen.h = as<Sint32>(window_W / aspect);
		} else {
			screen.h = window_H;
			screen.w = as<Sint32>(window_H / aspect);
		}
		*/
	}
}
