/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Host.hpp"

VM_Host::RenderSettings::RenderSettings()
    : emuVersion("[21.03.24]"s)
{}

bool VM_Host::RenderSettings::createWindow() {
    if (window) SDL_DestroyWindow(window);

    window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_W, window_H,
        SDL_WINDOW_RESIZABLE   |
        SDL_WINDOW_INPUT_FOCUS |
        SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (window) return true;
    errorMessage("Window init error"s);
    return false;
}

bool VM_Host::RenderSettings::createRenderer() {
    if (renderer) SDL_DestroyRenderer(renderer);

    renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED /*|
        SDL_RENDERER_PRESENTVSYNC*/
        // conflicts with the current frameLimiter setup
        // don't know how to marry the two..
    );

    if (renderer) return true;
    errorMessage("Renderer init error"s);
    return false;
}

void VM_Host::RenderSettings::changeTitle(const std::string_view name) {
    title  = emuVersion;
    title += " :: CubeChip :: "s;
    title += name;
    SDL_SetWindowTitle(window, title.data());
}

void VM_Host::RenderSettings::errorMessage(std::string&& newTitle) {
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        newTitle.data(),
        SDL_GetError(),
        window
    );
}

void VM_Host::RenderSettings::lockTexture() {
    void* pixel_ptr{ pixels };
    SDL_LockTexture(texture, nullptr, as<void**>(&pixel_ptr), &pitch);
    pixels = as<u32*>(pixel_ptr);
}

void VM_Host::RenderSettings::unlockTexture() {
    SDL_UnlockTexture(texture);
}
void VM_Host::RenderSettings::setTextureAlpha(const usz alpha) {
    SDL_SetTextureAlphaMod(texture, as<u8>(alpha));
}
void VM_Host::RenderSettings::setTextureBlend(const SDL_BlendMode blend) {
    SDL_SetTextureBlendMode(texture, blend);
}
void VM_Host::RenderSettings::present(const bool resize) {
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
        }
        else {
            screen.h = window_H;
            screen.w = as<s32>(window_H / aspect);
        }
        */
    }

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}
void VM_Host::RenderSettings::createTexture(const s32 length, const s32 width) {
    if (texture) SDL_DestroyTexture(texture);
    
    pitch = width * 4;

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_BGRA32,
        SDL_TEXTUREACCESS_STREAMING,
        width, length
    );
    present(true);
}
void VM_Host::RenderSettings::setAspectRatio(const float ratio) {
    aspect = ratio;
    // lackluster ain't it
}
