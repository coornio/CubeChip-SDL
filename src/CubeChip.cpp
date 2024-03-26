/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

//#include "Includes.hpp"
#include "HostClass/Host.hpp"
#include "GuestClass/Guest.hpp"



s32 SDL_main(s32 argc, char* argv[]) {
    std::unique_ptr<HomeDirManager> HDM;
    std::unique_ptr<BasicVideoSpec> BVS;
    std::unique_ptr<BasicAudioSpec> BAS;

    try {
        HDM = std::make_unique<HomeDirManager>("CubeChip_SDL");
        BVS = std::make_unique<BasicVideoSpec>(800, 400);
        BAS = std::make_unique<BasicAudioSpec>();
    }
    catch (...) { return EXIT_FAILURE; }

    VM_Host Host(
        *HDM.get(), *BVS.get(), *BAS.get(),
        argc > 1 ? argv[1] : nullptr
    );

    std::unique_ptr<VM_Guest> Guest;

    auto createGuest{ [](VM_Host& host) {
        return std::make_unique<VM_Guest>(host);
    } };

    FrameLimiter Frame;
    SDL_Event    Event;

    {
    reset_all:
        // do I even need it as a unique_ptr still?
        Guest = nullptr;
        Guest = createGuest(Host);

        // this segment is ugly, fix it fix it fix it
        if (Guest->setupMachine()) {
            Host.isReady(true);
            // need to update Frame object with program.framerate too
            BVS->changeTitle(HDM->name);
            BAS->pauseDevice(false);
            BAS->handler = [&](s16* buffer, const s32 frames) {
                Guest->Audio.renderAudio(buffer, frames);
            };
        }
        else {
            Host.isReady(false);
            Host.hasFile(false);
            HDM->reset();
            BAS->pauseDevice(true);
        }

        while (true) {
            while (SDL_PollEvent(&Event)) {
                switch (Event.type) {
                    case SDL_QUIT: {
                        // exit program
                        // will potentially still cause leaks
                        return EXIT_SUCCESS;
                    } break;
                    case SDL_DROPFILE: {
                        // until file validity check becomes valid, this leaks:
                        const std::string filedrop{ Event.drop.file };
                        SDL_free(Event.drop.file);

                        if (HDM->verifyFile(filedrop.c_str())) {
                            Host.hasFile(true);
                            Host.isReady(false);
                            BAS->pauseDevice(true);
                            goto reset_all;
                            // 1) scan file with static Guest function (soon?)
                            // 2) if valid: restart loop from the start
                        }
                    } break;
                    case SDL_WINDOWEVENT: {
                        switch (Event.window.event) {
                            case SDL_WINDOWEVENT_RESIZED: {
                                BVS->resizeWindow(Event.window.data1, Event.window.data2);
                                BVS->renderPresent();
                            } break;
                            case SDL_WINDOWEVENT_MINIMIZED:
                                break;
                            case SDL_WINDOWEVENT_RESTORED:
                                break;
                        }
                    } break;
                }
            }


            if (bic::kb.isPressed(KEY(ESCAPE))) {
                // exit program
                // will potentially still cause leaks
                return EXIT_SUCCESS;
            }
            if (bic::kb.isPressed(KEY(RSHIFT))) {
                Host.benchmarking = !Host.benchmarking;
            }
            if (bic::kb.isPressed(KEY(UP))) {
                BAS->setVolume(BAS->volume + 15);
            }
            if (bic::kb.isPressed(KEY(DOWN))) {
                BAS->setVolume(BAS->volume - 15);
            }

            if (!Frame(Host.benchmarking
                ? FrameLimiter::SPINLOCK
                : FrameLimiter::SLEEP
            )) continue;

            if (Host.isReady()) {
                if (bic::kb.isPressed(KEY(BACKSPACE))) {
                    BAS->pauseDevice(true);
                    goto reset_all;
                    // re-launch with current rom by
                    // destroying guest and re-creating
                }
                Guest->cycle();
            }
        }
    }
    return 0;
}
