/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicAudioSpec.hpp"
#include <algorithm>

/*------------------------------------------------------------------*/
/*  class  VM_Host::AudioSettings                                   */
/*------------------------------------------------------------------*/

static constexpr Sint32 VOL_MAX{ 255 };
static constexpr Sint32 VOL_MIN{ 0 };

BasicAudioSpec::BasicAudioSpec(const Sint32 freq)
    : outFrequency{ freq }
{
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    setVolume(VOL_MAX);
}

BasicAudioSpec::~BasicAudioSpec() {
    if (device) SDL_CloseAudioDevice(device);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void BasicAudioSpec::setSpec() {
    SDL_zero(spec);
    spec.freq     = outFrequency;
    spec.format   = AUDIO_S16SYS;
    spec.channels = 1;
    spec.samples  = 128;
    spec.callback = &audioCallback;
    spec.userdata = this;
    device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
}

void BasicAudioSpec::setVolume(const Sint32 vol) {
    volume = std::clamp(vol, VOL_MIN, VOL_MAX);
    amplitude = static_cast<Sint16>(16 * volume);
}

void BasicAudioSpec::changeVolume(const Sint32 delta) {
    volume = std::clamp(volume + delta, VOL_MIN, VOL_MAX);
    amplitude = static_cast<Sint16>(16 * volume);
}

void BasicAudioSpec::pauseDevice(const bool state) {
    SDL_PauseAudioDevice(device, state);
}

void BasicAudioSpec::audioCallback(void* data, Uint8* buffer, const Sint32 bytes) {
    auto* self = reinterpret_cast<BasicAudioSpec*>(data);
    if (self->handler)
        self->handler(reinterpret_cast<Sint16*>(buffer), bytes >> 1);
}
