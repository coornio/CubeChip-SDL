/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

//#include "Includes.hpp"
#include "HostClass/Host.hpp"
#include "GuestClass/Guest.hpp"

int32_t SDL_main(int32_t argc, char* argv[]) {
    std::unique_ptr<HomeDirManager> HDM;
    std::unique_ptr<BasicVideoSpec> BVS;
    std::unique_ptr<BasicAudioSpec> BAS;

    try {
        HDM = std::make_unique<HomeDirManager>("CubeChip_SDL");
        BVS = std::make_unique<BasicVideoSpec>(800, 400);
        BAS = std::make_unique<BasicAudioSpec>(48'000);
    }
    catch (...) {
        SDL_Quit();
        return EXIT_FAILURE;
    }

    VM_Host Host(
        *HDM.get(),
        *BVS.get(),
        *BAS.get()
    );

    BAS->setSpec();

    if (HDM->verifyFile(argc > 1 ? argv[1] : nullptr)) {
        Host.isReady(true);
    }

    std::unique_ptr<VM_Guest> Guest;

    FrameLimiter Frame;
    SDL_Event    Event;

    {
    reset_all:
        //VM_Guest Guest(Host);

        bic::kb.updateCopy();
        bic::mb.updateCopy();

        if (Host.isReady()) {
            // placeholder to avoid duplication from goto (for now)
            Guest = nullptr; // destroys previous object
            Guest = std::make_unique<VM_Guest>(
                *HDM.get(),
                *BVS.get(),
                *BAS.get()
            );

            // this segment is ugly, fix it fix it fix it
            if (Guest->setupMachine()) {
                // need to update Frame object with program.framerate too
                BVS->changeTitle(HDM->name.c_str());
                BAS->setHandler(Guest);
            }
            else {
                Host.isReady(false);
                HDM->reset();
                BAS->pauseDevice(true);
            }
        }
        // initialization area above ^

        // loops below

        while (true) {
            while (SDL_PollEvent(&Event)) {
                switch (Event.type) {
                    case SDL_QUIT: {
                        SDL_Quit();
                        return EXIT_SUCCESS;
                    } break;
                    case SDL_DROPFILE: {
                        // until file validity check becomes static, this leaks:
                        const std::string filedrop{ Event.drop.file };
                        SDL_free(Event.drop.file);

                        // currently file verification wipes old data, should fix
                        if (HDM->verifyFile(filedrop.c_str())) {
                            blog.stdLogOut("File drop accepted: " + filedrop);
                            Host.isReady(true);
                            BAS->pauseDevice(true);
                            goto reset_all;
                        }
                        else {
                            blog.stdLogOut("File drop denied: " + filedrop);
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

            if (!Frame(Host.doBench()
                ? FrameLimiter::SPINLOCK
                : FrameLimiter::SLEEP
            )) continue;

            if (bic::kb.isPressed(KEY(ESCAPE))) {
                if (Host.isReady()) {
                    Host.isReady(false);
                    BVS->changeTitle();
                    BVS->renderPresent(); // should black out screen again
                    BAS->pauseDevice(true);
                    goto reset_all;
                }
                else {
                    SDL_Quit();
                    return EXIT_SUCCESS;
                }
            }
            if (bic::kb.isPressed(KEY(RSHIFT))) {
                Host.doBench(!Host.doBench());
            }
            if (bic::kb.isPressed(KEY(UP))) {
                BAS->changeVolume(+15);
            }
            if (bic::kb.isPressed(KEY(DOWN))) {
                BAS->changeVolume(-15);
            }

            if (Host.isReady()) {
                if (bic::kb.isPressed(KEY(BACKSPACE))) {
                    BAS->pauseDevice(true);
                    goto reset_all;
                }
                Guest->cycle();
            }

            bic::kb.updateCopy();
            bic::mb.updateCopy();
        }
    }
}
