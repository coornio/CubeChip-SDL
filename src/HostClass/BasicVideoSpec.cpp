/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <stdexcept>
#include <algorithm>

#include "BasicVideoSpec.hpp"

BasicVideoSpec::BasicVideoSpec()
	: enableBuzzGlow{ true }
{
	try {
		SDL_InitSubSystem(SDL_INIT_VIDEO);
		createWindow(0, 0);
		createRenderer();
		resetWindow();
	}
	catch (const std::exception& e) {
		showErrorBoxSDL(e.what());
		throw;
	}
}

BasicVideoSpec::~BasicVideoSpec() {
	quitTexture();
	quitRenderer();
	quitWindow();
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void BasicVideoSpec::createWindow(const Sint32 window_W, const Sint32 window_H) {
	quitWindow();

	window = SDL_CreateWindow(nullptr, window_W, window_H, 0);

	if (!window) {
		throw std::runtime_error(SDL_GetError());
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
		throw std::runtime_error(SDL_GetError());
	}
}

void BasicVideoSpec::createTexture(Sint32 texture_W, Sint32 texture_H) {
	quitTexture();

	if (!texture_W || !texture_H) { return; }

	texture_W = std::max<Sint32>(std::abs(texture_W),  1);
	texture_H = std::max<Sint32>(std::abs(texture_H), 1);

	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		texture_W, texture_H
	);

	if (!texture) {
		throw std::runtime_error(SDL_GetError());
	} else {
		SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
		ppitch = texture_W * 4;
	}
}

void BasicVideoSpec::changeTitle(const char* name) {
	static constexpr char emuVersion[]{ "[06.06.24]" };
	std::string windowTitle{ "CubeChip :: " };
	windowTitle += (name ? name : "Waiting for file...");
	SDL_SetWindowTitle(window, windowTitle.c_str());
}

bool BasicVideoSpec::showErrorBoxSDL(std::string_view title) {
	return SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title.data(),
		SDL_GetError(), nullptr
	);
}

bool BasicVideoSpec::showErrorBox(std::string_view message, std::string_view title) {
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

void BasicVideoSpec::resetWindow() {
	SDL_SetWindowSize(window, 640, 480);
	changeTitle();
	createTexture();
	renderPresent();
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
	const Sint32 texture_W,
	const Sint32 texture_H,
	const Sint32 padding_S
) {
	const auto padding_A{ std::abs(padding_S) };

	PerimeterWidth = padding_A;
	enableScanLine = padding_A == padding_S;

	frameGame = {
		static_cast<float>(padding_A),
		static_cast<float>(padding_A),
		static_cast<float>(texture_W),
		static_cast<float>(texture_H)
	};

	frameFull.w = texture_W + 2.0f * PerimeterWidth;
	frameFull.h = texture_H + 2.0f * PerimeterWidth;

	multiplyWindowDimensions();

	SDL_SetRenderLogicalPresentation(
		renderer,
		texture_W + PerimeterWidth * 2,
		texture_H + PerimeterWidth * 2,
		SDL_LOGICAL_PRESENTATION_INTEGER_SCALE,
		SDL_SCALEMODE_NEAREST
	);
}

void BasicVideoSpec::multiplyWindowDimensions() {
	const auto window_W{ static_cast<Sint32>(frameFull.w) };
	const auto window_H{ static_cast<Sint32>(frameFull.h) };

	SDL_SetWindowMinimumSize(window, window_W, window_H);
	SDL_SetWindowSize(window, window_W * frameMultiplier, window_H * frameMultiplier);
}

void BasicVideoSpec::changeFrameMultiplier(const Sint32 delta) {
	frameMultiplier = std::clamp(frameMultiplier + delta, 1, 8);
	multiplyWindowDimensions();
}

void BasicVideoSpec::renderPresent() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);

	if (texture) {
		SDL_SetRenderDrawColor(
			renderer,
			PerimeterColor.r, PerimeterColor.g,
			PerimeterColor.b, PerimeterColor.a
		);
		SDL_RenderFillRect(renderer, &frameFull);

		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
		SDL_RenderTexture(renderer, texture, nullptr, &frameGame);

		if (enableScanLine) {
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 32);

			const auto drawLimit{ static_cast<Sint32>(frameFull.h) };
			for (auto y{ 0 }; y < drawLimit; y += PerimeterWidth) {
				SDL_RenderLine(
					renderer,
					frameFull.x, static_cast<float>(y),
					frameFull.w, static_cast<float>(y)
				);
			}
		}
	} else {
		SDL_RenderTexture(renderer, nullptr, nullptr, nullptr);
	}

	SDL_RenderPresent(renderer);
}

void BasicVideoSpec::quitTexture() {
	if (texture) {
		SDL_DestroyTexture(texture);
		texture = nullptr;
	}
}
void BasicVideoSpec::quitRenderer() {
	if (renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}
}
void BasicVideoSpec::quitWindow() {
	if (window) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}
}
