
#include "../GuestClass/Guest.hpp"
#include "Host.hpp"

/*------------------------------------------------------------------*/
/*  class  VM_Host                                                  */
/*------------------------------------------------------------------*/

VM_Host::VM_Host(const char* path) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    if (!Render.createWindow())   goto fail;
    Render.changeTitle("Waiting"s);
    if (!Render.createRenderer()) goto fail;

    Audio.setSpec(this);
    File.verifyFile(path);

    machineLoaded = true;
    return;
fail:
    machineLoaded = false;
}

VM_Host::~VM_Host() {
    if (Audio.device)    SDL_CloseAudioDevice(Audio.device);
    if (Render.texture)  SDL_DestroyTexture(Render.texture);
    if (Render.renderer) SDL_DestroyRenderer(Render.renderer);
    if (Render.window)   SDL_DestroyWindow(Render.window);
    SDL_Quit();
}

bool VM_Host::machineValid() const { return machineLoaded; }
bool VM_Host::programValid() const { return programLoaded; }

void VM_Host::addMessage(const std::string_view msg, const bool header, const u32 code) {
    if (header) {
        std::cout << msg << std::endl;
        return;
    }
    if (!code) {
        std::cout << "  >>  "s << msg << std::endl;
        return;
    }
    std::cout << "  >>  "s         << msg << " 0x"s
              << std::setfill('0') << std::setw(4)
              << std::uppercase    << std::hex
              << code              << std::endl;
}

void VM_Host::runMachine(VM_Guest& vm) {
    SDL_Event event{};

    FrameLimiter Frame(vm.Program.framerate);

    Audio.handler = [&](s16* buffer, const u32 frames) {
        vm.Audio.renderAudio(buffer, frames);
    };
    SDL_PauseAudioDevice(Audio.device, 0);

    while (true) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    programLoaded = true;
                    goto exit;
                } break;
                case SDL_KEYDOWN: {
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_ESCAPE:
                            programLoaded = true;
                            goto exit;
                        case SDL_SCANCODE_BACKSPACE:
                            programLoaded = false;
                            goto exit;
                    }
                } break;
                case SDL_DROPFILE: {
                    const bool dropSuccess{ File.verifyFile(event.drop.file) };
                    SDL_free(event.drop.file);
                    if (dropSuccess) {
                        addMessage("Hotswapping ROM: "s + File.name);
                        programLoaded = false;
                        goto exit;
                    }
                } break;
                case SDL_WINDOWEVENT: {
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_RESIZED: {
                            SDL_GetWindowSize(
                                Render.window,
                                &Render.window_W,
                                &Render.window_H
                            );
                            Render.present(true);
                        }
                    }
                } break;
            }
        }

        if (benchmarking) {
            if (!Frame(SPINLOCK)) continue;
            std::cout << "\33[1;1H" << (cycles++ / 60.0f) << std::endl;
            std::cout << "cycles: " << cycles
                << "\nipf: " << vm.Program.ipf
                << std::endl;

            if (!Frame.paced())
                std::cout << "cannot keep up!!";
            else
                std::cout << "keeping up pace.";

            std::cout << "\ntime since last frame: " << Frame.elapsed();
        } else if (!Frame(SLEEP)) continue;

        using namespace bki;
        if (kb.isKeyPressed(SDL_SCANCODE_RSHIFT)) {
            benchmarking = !benchmarking;
        }
        if (kb.isKeyPressed(SDL_SCANCODE_UP)) {
            Audio.setVolume(Audio.volume + 0.1f);
        }
        if (kb.isKeyPressed(SDL_SCANCODE_DOWN)) {
            Audio.setVolume(Audio.volume - 0.1f);
        }
        kb.updateKeyboardCopy();
        vm.cycle();
    }
exit:
    SDL_PauseAudioDevice(Audio.device, 1);
}

/*------------------------------------------------------------------*/
/*  class  VM_Host::RomInfo                                         */
/*------------------------------------------------------------------*/

// _RomInfo.cpp

/*------------------------------------------------------------------*/
/*  class  VM_Host::RenderSettings                                  */
/*------------------------------------------------------------------*/

// _RenderSettings.cpp

/*------------------------------------------------------------------*/
/*  class  VM_Host::AudioSettings                                   */
/*------------------------------------------------------------------*/

// _AudioSettings.cpp
