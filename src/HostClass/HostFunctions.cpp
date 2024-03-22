/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../GuestClass/Guest.hpp"
#include "Host.hpp"

/*------------------------------------------------------------------*/
/*  class  VM_Host                                                  */
/*------------------------------------------------------------------*/

VM_Host::VM_Host(const char* path) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    if (!Render.createWindow())   return;
    if (!Render.createRenderer()) return;
    if (!File.verifyHome())       return;
    if (!File.verifyFile(path))   return;
    
    Render.changeTitle("Waiting"sv);
    Audio.setSpec(this);

    machineLoaded = true;
    return;
}

VM_Host::~VM_Host() {
    if (Audio.device)    SDL_CloseAudioDevice(Audio.device);
    Render.quitSDL();
}

bool VM_Host::machineValid() const { return machineLoaded; }
bool VM_Host::programValid() const { return programLoaded; }

void VM_Host::addMessage(const std::string_view msg, const bool header, const usz code) {
    if (header) {
        std::cout << msg << std::endl;
        return;
    }
    if (!code) {
        std::cout << "  >>  "sv << msg << std::endl;
        return;
    }
    std::cout << "  >>  "sv        << msg << " 0x"sv
              << std::setfill('0') << std::setw(4)
              << std::uppercase    << std::hex
              << code              << std::endl;
}

void VM_Host::runMachine(VM_Guest& vm) {
    SDL_Event    event;
    FrameLimiter Frame(vm.Program.framerate);

    using namespace bic;

    Audio.handler = [&](s16* buffer, const s32 frames) {
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
                            Render.getWindowSize(true);
                            Render.renderPresent();
                        }
                    }
                } break;
            }
        }

        if (benchmarking) {
            if (!Frame(FrameLimiter::SPINLOCK)) continue;
            std::cout << "\33[1;1H" << (cycles++ / 60.0f) << std::endl;
            std::cout << "cycles: " << cycles
                << "\nipf: " << vm.Program.ipf
                << std::endl;

            if (!Frame.paced())
                std::cout << "cannot keep up!!" << std::endl;
            else
                std::cout << "keeping up pace." << std::endl;

            std::cout << "time since last frame: " << Frame.elapsed() << std::endl;
        } else if (!Frame(FrameLimiter::SLEEP)) continue;

        if (kb.isPressed(KEY(BACKSPACE))) {
            programLoaded = false;
            goto exit;
        }
        if (kb.isPressed(KEY(ESCAPE))) {
            programLoaded = true;
            goto exit;
        }
        if (kb.isPressed(KEY(RSHIFT))) {
            benchmarking = !benchmarking;
        }
        if (kb.isPressed(KEY(UP))) {
            Audio.setVolume(Audio.volume + 15);
        }
        if (kb.isPressed(KEY(DOWN))) {
            Audio.setVolume(Audio.volume - 15);
        }
        mb.updateCopy();
        kb.updateCopy();
        vm.cycle();
    }
exit:
    kb.updateCopy();
    SDL_PauseAudioDevice(Audio.device, 1);
}

/*------------------------------------------------------------------*/
/*  class  VM_Host::FileInfo                                        */
/*------------------------------------------------------------------*/

// _FileInfo.cpp

/*------------------------------------------------------------------*/
/*  class  VM_Host::AudioSettings                                   */
/*------------------------------------------------------------------*/

// _AudioSettings.cpp
