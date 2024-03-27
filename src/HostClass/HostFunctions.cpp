/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Host.hpp"

/*------------------------------------------------------------------*/
/*  class  VM_Host                                                  */
/*------------------------------------------------------------------*/

VM_Host::VM_Host(
    HomeDirManager& hdm_ptr,
    BasicVideoSpec& bvs_ptr,
    BasicAudioSpec& bas_ptr
)
    : File{ hdm_ptr }
    , Video{ bvs_ptr }
    , Audio{ bas_ptr }
{
    /*
    Video.changeTitle("Waiting for file...");
    Audio.setSpec();

    if (File.verifyFile(path)) {
        hasFile(true).isReady(false);
    }
    */
}

bool VM_Host::isReady() const { return _isReady; }
bool VM_Host::doBench() const { return _doBench; }

VM_Host& VM_Host::isReady(const bool state) { _isReady = state; return *this; }
VM_Host& VM_Host::doBench(const bool state) { _doBench = state; return *this; }

/*
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
*/
