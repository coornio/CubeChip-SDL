/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdexcept>

#include "BasicVideoSpec.hpp"

BasicVideoSpec::BasicVideoSpec(const Sint32 w, const Sint32 h)
	: emuName   { "CubeChip" }
	, emuVersion{ "[06.06.24]" }
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
		// will need to thread things out, etc.
	);
	
	if (!renderer) {
		throw std::runtime_error("SDL Error: Renderer");
	}
}

void BasicVideoSpec::createTexture(Sint32 width, Sint32 height) {
	quitTexture();

	if (!width || !height) {
		texture = nullptr;
		return;
	} else {
		width  = std::max<Sint32>(std::abs(width),  1);
		height = std::max<Sint32>(std::abs(height), 1);
	}

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
	//windowTitle  = emuVersion + " :: ";
	windowTitle  = emuName    + " :: ";
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

void BasicVideoSpec::AudioOutline(const Uint32 color_1, const Uint32 color_0) {
	PerimeterColor = {
		static_cast<Uint8>(((enableBuzzGlow ? color_1 : color_0) >> 16)),
		static_cast<Uint8>(((enableBuzzGlow ? color_1 : color_0) >>  8)),
		static_cast<Uint8>( (enableBuzzGlow ? color_1 : color_0)       ),
		SDL_ALPHA_OPAQUE
	};
}

void BasicVideoSpec::raiseWindow() {
	SDL_RaiseWindow(window);
}
void BasicVideoSpec::lockTexture() {
	void* pixel_ptr{ pixels };
	SDL_LockTexture(
		texture, nullptr,
		&pixel_ptr,
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

void BasicVideoSpec::setAspectRatio(
	const Sint32 width,
	const Sint32 height,
	const Sint32 size
) {
	const auto oSize{ std::abs(size) };

	frameWidth = oSize;
	scanLineOn = oSize == size;

	frameGame = {
		static_cast<float>(oSize),
		static_cast<float>(oSize),
		static_cast<float>(width),
		static_cast<float>(height)
	};

	frameFull.w = width  + 2.0f * frameWidth;
	frameFull.h = height + 2.0f * frameWidth;

	SDL_SetWindowMinimumSize(
		window,
		static_cast<Sint32>(frameFull.w),
		static_cast<Sint32>(frameFull.h)
	);

	SDL_SetRenderLogicalPresentation(
		renderer,
		width  + frameWidth * 2,
		height + frameWidth * 2,
		SDL_LOGICAL_PRESENTATION_INTEGER_SCALE,
		SDL_SCALEMODE_NEAREST
	);
}

void BasicVideoSpec::resizeWindow(Sint32 W, Sint32 H) {
	SDL_SetWindowSize(window, W, H);
}

void BasicVideoSpec::renderPresent() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);

	if (texture) {
		SDL_SetRenderDrawColor(
			renderer,
			frameColor.r, frameColor.g,
			frameColor.b, frameColor.a
		);
		SDL_RenderFillRect(renderer, &frameFull);

		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
		SDL_RenderTexture(renderer, texture, nullptr, &frameGame);

		if (scanLineOn) {
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 32);

			for (auto y{ 0 }; y < static_cast<Sint32>(frameFull.h); y += frameWidth) {
				SDL_RenderLine(renderer, frameFull.x, static_cast<float>(y), frameFull.w, static_cast<float>(y));
			}
		}
	} else {
		SDL_RenderTexture(renderer, nullptr, nullptr, nullptr);
	}

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
