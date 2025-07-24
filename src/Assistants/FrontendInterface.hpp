/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

/*==================================================================*/

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Window;
union SDL_Event;

class FrontendInterface {

public:
	static inline void (*FnHook_OpenFile)() {};

public:
	static void Initialize(SDL_Window*, SDL_Renderer*);
	static void Shutdown();

	static void ProcessEvent(SDL_Event* event);
	static void NewFrame();
	static void RenderFrame(SDL_Renderer*);

	static float GetFrameHeight();

	static void UpdateFontScale(const void* data, int size, float scale);

	static void PrepareViewport(
		bool enable, bool integer_scaling,
		int width, int height, int rotation,
		const char* overlay, SDL_Texture* texture
	);
	static void PrepareGeneralUI();
};
