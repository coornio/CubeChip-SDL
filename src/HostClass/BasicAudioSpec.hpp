/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL3/SDL.h>
#include <cstddef>

class BasicAudioSpec final {
public:
    const Sint32 outFrequency;
    Sint32 volume{};
    float  bytesLeft{};
    Sint16 amplitude{};

private:
    SDL_AudioSpec     spec{};
    SDL_AudioStream*  stream{};
    SDL_AudioDeviceID device{};
    
public:
    explicit BasicAudioSpec(const Sint32);
    ~BasicAudioSpec();

    void pushAudioData(const void* const, const std::size_t);

    void setVolume(const Sint32);
    void changeVolume(const Sint32);
};
