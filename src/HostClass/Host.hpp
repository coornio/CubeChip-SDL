/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL.h>

#include "HomeDirManager.hpp"
#include "BasicVideoSpec.hpp"
#include "BasicAudioSpec.hpp"

class VM_Host final {
    bool _isReady{};
    bool _doBench{};

public:
    [[maybe_unused]] \
    unsigned long long cycles{};

    HomeDirManager& File;
    BasicVideoSpec& Video;
    BasicAudioSpec& Audio;

    explicit VM_Host(
        HomeDirManager&,
        BasicVideoSpec&,
        BasicAudioSpec&
    );

    [[nodiscard]] bool isReady() const;
    [[nodiscard]] bool doBench() const;
    VM_Host& isReady(const bool);
    VM_Host& doBench(const bool);
};
