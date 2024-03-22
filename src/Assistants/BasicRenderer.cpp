/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../Includes.hpp"

BasicRenderer::BasicRenderer()
	: window_W(800) // placeholder
	, window_H(400) // placeholder
{}

bool BasicRenderer::createWindow() {
	quitWindow();

	window = SDL_CreateWindow(
		windowTitle.c_str(),
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		window_W, window_H,
		SDL_WINDOW_RESIZABLE |
		SDL_WINDOW_INPUT_FOCUS |
		SDL_WINDOW_ALLOW_HIGHDPI
	);

	if (window) return true;
	errorMessage("Window init error");
	return false;
}

bool BasicRenderer::createRenderer() {
	quitRenderer();

	renderer = SDL_CreateRenderer(
		window, -1,
		SDL_RENDERER_ACCELERATED /*|
		SDL_RENDERER_PRESENTVSYNC*/
		// conflicts with the current frameLimiter setup
		// don't know how to marry the two..
	);

	if (renderer) return true;
	errorMessage("Renderer init error");
	return false;
}

bool BasicRenderer::createTexture(const s32 width, const s32 height) {
	quitTexture();
	ppitch = width * 4;

	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		width, height
	);
	if (texture) return true;

	errorMessage("Texture init error");
	return false;
}

void BasicRenderer::changeTitle(const std::string_view name) {
	windowTitle  = emuVersion + " :: ";
	windowTitle += emuName    + " :: ";
	windowTitle += name;
	SDL_SetWindowTitle(window, windowTitle.c_str());
}

void BasicRenderer::errorMessage(std::string&& newTitle) {
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, newTitle.c_str(),
		SDL_GetError(), window
	);
}

void BasicRenderer::lockTexture() {
	void* pixel_ptr{ pixels };
	SDL_LockTexture(texture, nullptr, as<void**>(&pixel_ptr), &ppitch);
	pixels = as<u32*>(pixel_ptr);
}
void BasicRenderer::unlockTexture() {
	SDL_UnlockTexture(texture);
}

void BasicRenderer::setTextureAlpha(const usz alpha) {
	SDL_SetTextureAlphaMod(texture, as<u8>(alpha));
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
void BasicRenderer::quitSDL() {
	quitTexture();
	quitRenderer();
	quitWindow();
	SDL_Quit();
}

void BasicRenderer::getWindowSize(const bool resize) {
	// const auto old_W{ window_W }, old_H{ window_H };
	SDL_GetWindowSize(window, &window_W, &window_H);

	if (resize) {
		window_W &= 0x0FFFFFFC;
		window_W = std::max(window_W, 640);
		window_H = as<s32>(window_W / aspect);
		SDL_SetWindowSize(window, window_W, window_H);
		/*
		const float aspect_window =
			window_W / 1.0f / window_H;

		if (aspect > aspect_window) {
			screen.w = window_W;
			screen.h = as<s32>(window_W / aspect);
		} else {
			screen.h = window_H;
			screen.w = as<s32>(window_H / aspect);
		}
		*/
	}
}
