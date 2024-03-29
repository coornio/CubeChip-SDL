/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL.h>
#include <functional>

class BasicAudioSpec final {
    SDL_AudioDeviceID device{};
    SDL_AudioSpec     spec{};

public:
    std::function<void(Sint16*, Uint32)> handler;
    const Sint32 outFrequency;

    Sint32 volume{};
    Sint16 amplitude{};

    // this ideally should offer more options but we'll make do for now
    explicit BasicAudioSpec(const Sint32 = 44'100);
    ~BasicAudioSpec();

    void setSpec();
    void setVolume(const Sint32);
    void changeVolume(const Sint32);
    void pauseDevice(const bool);

    template <class T>
    void setHandler(T& obj) {
        handler = [&obj](Sint16* buffer, const Sint32 frames) {
            obj->renderAudio(buffer, frames);
        };
        pauseDevice(false);
    }

    static void audioCallback(void*, Uint8*, Sint32);
};
