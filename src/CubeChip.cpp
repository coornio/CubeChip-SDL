/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

//#include "Includes.hpp"
#include "HostClass/Host.hpp"
#include "GuestClass/Guest.hpp"

s32 SDL_main(s32 argc, char* argv[]) {
 
    VM_Host host((argc > 1) ? argv[1] : nullptr);
    if (!host.machineValid()) return -1;

    while (!host.programValid()) {
        VM_Guest guest(host);
        if (!guest.setupMachine()) return -2;
        
        host.runMachine(guest);
    }
    return 0;
}
