/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

//#include "Includes.hpp"
#include "HostClass/Host.hpp"
#include "GuestClass/Guest.hpp"

s32 SDL_main(s32 argc, char* argv[]) {
    bool  hostInitialized{ false };
    char* hostFileArgument{ argc > 1 ? argv[1] : nullptr };

    std::unique_ptr<HomeDirManager> FileHostPtr;
    std::unique_ptr<BasicRenderer>  BasicRendererPtr;

    try {
        FileHostPtr      = std::make_unique<HomeDirManager>("CubeChip_SDL");
        BasicRendererPtr = std::make_unique<BasicRenderer>(800, 400);
    }
    catch (...) { return 1; }

    VM_Host host(
        *FileHostPtr.get(),
        *BasicRendererPtr.get(),
        hostFileArgument
    );
    //if (!host.isReady()) return -1;

    while (!host.romLoaded()) {
        VM_Guest guest(host);
        if (!guest.setupMachine()) return -2;
        
        host.runMachine(guest);
    }

    SDL_Quit();
    return 0;
}
