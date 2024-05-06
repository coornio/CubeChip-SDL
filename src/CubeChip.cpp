/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Includes.hpp"

#include "GuestClass/SoundCores.hpp" // this shouldn't be needed, see BAS header

int32_t SDL_main(int32_t argc, char* argv[]) {

    #ifdef _DEBUG
        SDL_version compiled{};
        SDL_version linked{};

        SDL_VERSION(&compiled);
        SDL_GetVersion(&linked);

        printf("Compiled against SDL version %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch);
        printf("Linked SDL version %d.%d.%d\n", linked.major, linked.minor, linked.patch);
    #endif

    std::unique_ptr<HomeDirManager> HDM;
    std::unique_ptr<BasicVideoSpec> BVS;
    std::unique_ptr<BasicAudioSpec> BAS;
    std::unique_ptr<VM_Guest>       Guest;
    std::string                     filedrop;

    try {
        HDM = std::make_unique<HomeDirManager>("CubeChip_SDL");
        BVS = std::make_unique<BasicVideoSpec>(800, 400);
        BAS = std::make_unique<BasicAudioSpec>(48'000);
    }
    catch (...) {
        SDL_Quit();
        return EXIT_FAILURE;
    }

    VM_Host      Host(HDM.get(), BVS.get(), BAS.get());
    FrameLimiter Frame;
    SDL_Event    Event;

    if (HDM->verifyFile(argc > 1 ? argv[1] : nullptr)) {
        Host.isReady(true);
    }

    {
    reset_all:
        bic::kb.updateCopy();
        bic::mb.updateCopy();

        if (Host.isReady()) {
            // destroy old instance, and create a new one
            Guest = nullptr;
            Guest = std::make_unique<VM_Guest>(HDM.get(), BVS.get(), BAS.get());

            if (Guest->setupMachine()) {
                // need to adjust the FrameLimiter object with new framerate
                BVS->changeTitle(HDM->file.c_str());
                BAS->setHandler(Guest->Sound);
            }
            else {
                Host.isReady(false);
                HDM->reset();
                BAS->pauseDevice(true);
            }
        }

        while (true) {
            while (SDL_PollEvent(&Event)) {
                switch (Event.type) {
                    case SDL_QUIT: {
                        SDL_Quit();
                        return EXIT_SUCCESS;
                    } break;
                    case SDL_DROPFILE: {
                        filedrop = Event.drop.file;
                        SDL_free(Event.drop.file);

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
                            case SDL_WINDOWEVENT_SIZE_CHANGED:
                                BVS->resizeWindow(Event.window.data1, Event.window.data2);
                                BVS->renderPresent();
                                BAS->pauseDevice(false);
                                break;
                            case SDL_WINDOWEVENT_MOVED:
                                BAS->pauseDevice(false);
                                break;
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
                    BVS->createTexture(1, 1);
                    BVS->renderPresent();
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
