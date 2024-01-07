
#include "Host.hpp"

bool VM_Host::RenderSettings::createWindow() {
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
    errorMessage("Window init error");
    return false;
}

bool VM_Host::RenderSettings::createRenderer() {
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

void VM_Host::RenderSettings::changeTitle(std::string_view name) {
    title = emuVersion;
    title += " :: CubeChip :: "s;
    title += name;
    SDL_SetWindowTitle(window, title.data());
}

void VM_Host::RenderSettings::errorMessage(const char* newTitle) {
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        newTitle, SDL_GetError(),
        window
    );
}

void VM_Host::RenderSettings::lockTexture() {
    SDL_LockTexture(
        texture, nullptr,
        reinterpret_cast<void**>(&pixels),
        &pitch
    );
}

void VM_Host::RenderSettings::unlockTexture() {
    SDL_UnlockTexture(texture);
}
void VM_Host::RenderSettings::setTextureAlpha(u8 alpha) {
    SDL_SetTextureAlphaMod(texture, alpha);
}
void VM_Host::RenderSettings::setTextureBlend(SDL_BlendMode blend) {
    SDL_SetTextureBlendMode(texture, blend);
}
void VM_Host::RenderSettings::present(bool resize) {
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
void VM_Host::RenderSettings::setTexture(const s16 length, const s16 width, const float ratio) {
    pitch = width * 4;
    aspect = ratio;

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_BGRA32,
        SDL_TEXTUREACCESS_STREAMING,
        width, length
    );
    present(true);
}
