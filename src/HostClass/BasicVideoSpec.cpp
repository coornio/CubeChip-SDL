/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicVideoSpec.hpp"
#include <stdexcept>

BasicVideoSpec::BasicVideoSpec(const Sint32 w, const Sint32 h)
	: emuName{ "CubeChip" }
	, emuVersion{ "[09.05.24]" }
{
	try {
		SDL_InitSubSystem(SDL_INIT_VIDEO);
		createWindow(w, h);
		createRenderer();
	}
	catch (const std::exception& e) {
		showErrorBoxSDL(e.what());
		throw;
	}
	changeTitle();
}

BasicVideoSpec::~BasicVideoSpec() {
	quitTexture();
	quitRenderer();
	quitWindow();
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void BasicVideoSpec::createWindow(
	const Sint32 window_W,
	const Sint32 window_H
) {
	quitWindow();

	window = SDL_CreateWindow(
		windowTitle.c_str(),
		window_W, window_H,
		SDL_WINDOW_RESIZABLE
	);

	if (!window) {
		throw std::runtime_error("SDL Error: Window");
	}
}

void BasicVideoSpec::createRenderer() {
	quitRenderer();

	renderer = SDL_CreateRenderer(
		window, nullptr, 0
		/*SDL_RENDERER_PRESENTVSYNC*/
		// conflicts with the current frameLimiter setup
		// don't know how to marry the two..
	);

	if (!renderer) {
		throw std::runtime_error("SDL Error: Renderer");
	}
}

void BasicVideoSpec::createTexture(const Sint32 width, const Sint32 height) {
	quitTexture();

	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		width, height
	);

	if (!texture) {
		throw std::runtime_error("SDL Error: Texture");
	} else {
		SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
		ppitch = width * 4;
	}
}

void BasicVideoSpec::changeTitle(const char* name) {
	windowTitle  = emuVersion + " :: ";
	windowTitle += emuName    + " :: ";
	windowTitle += (name ? name : "Waiting for file...");
	SDL_SetWindowTitle(window, windowTitle.c_str());
}

bool BasicVideoSpec::showErrorBoxSDL(
	std::string_view title
) {
	return SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title.data(),
		SDL_GetError(), nullptr
	);
}

bool BasicVideoSpec::showErrorBox(
	std::string_view message,
	std::string_view title
) {
	return SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title.data(),
		message.data(), nullptr
	);
}

void BasicVideoSpec::lockTexture() {
	void* pixel_ptr{ pixels };
	SDL_LockTexture(
		texture, nullptr,
		static_cast<void**>(&pixel_ptr),
		&ppitch
	);
	pixels = static_cast<Uint32*>(pixel_ptr);
}
void BasicVideoSpec::unlockTexture() {
	SDL_UnlockTexture(texture);
}

void BasicVideoSpec::setTextureAlpha(const std::size_t alpha) {
	SDL_SetTextureAlphaMod(texture, static_cast<Uint8>(alpha));
}
void BasicVideoSpec::setTextureBlend(const SDL_BlendMode blend) {
	SDL_SetTextureBlendMode(texture, blend);
}

void BasicVideoSpec::setAspectRatio(const std::pair<Sint32, Sint32> res) {
	SDL_SetRenderLogicalPresentation(
		renderer, 
		res.first, 
		res.second, 
		SDL_LOGICAL_PRESENTATION_LETTERBOX, 
		SDL_SCALEMODE_NEAREST
	);
}
void BasicVideoSpec::resizeWindow(Sint32 W, Sint32 H) {
	SDL_SetWindowSize(window, W, H);
}

void BasicVideoSpec::renderPresent() {
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, texture, nullptr, nullptr);
	SDL_RenderPresent(renderer);
}

void BasicVideoSpec::quitTexture() {
	if (texture)  SDL_DestroyTexture(texture);
}
void BasicVideoSpec::quitRenderer() {
	if (renderer) SDL_DestroyRenderer(renderer);
}
void BasicVideoSpec::quitWindow() {
	if (window)   SDL_DestroyWindow(window);
}
