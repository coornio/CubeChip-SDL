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

VM_Host::VM_Host(
    HomeDirManager& hdm_ptr,
    BasicVideoSpec& bvs_ptr,
    BasicAudioSpec& bas_ptr,
    const char* path
)
    : File{ hdm_ptr }
    , Video{ bvs_ptr }
    , Audio{ bas_ptr }
{
    Video.changeTitle("Waiting for file...");
    Audio.setSpec(this);

    if (File.verifyFile(path)) {
        hasFile(true);
        isReady(false);
    }
}

bool VM_Host::isReady() const { return _isReady; }
bool VM_Host::hasFile() const { return _hasFile; }

void VM_Host::isReady(const bool state) { _isReady = state; }
void VM_Host::hasFile(const bool state) { _hasFile = state; }

/*
void VM_Host::runMachine(VM_Guest& vm) {
    SDL_Event    event;
    FrameLimiter Frame(vm.Program.framerate);

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
                        blog.stdLogOut("Hotswapping ROM : " + File.name);
                        programLoaded = false;
                        goto exit;
                    }
                } break;
                case SDL_WINDOWEVENT: {
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_RESIZED: {
                            Video.resizeWindow(true);
                            Video.renderPresent();
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
*/
