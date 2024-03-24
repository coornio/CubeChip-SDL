/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Host.hpp"

/*------------------------------------------------------------------*/
/*  class  VM_Host::AudioSettings                                   */
/*------------------------------------------------------------------*/

VM_Host::AudioSettings::AudioSettings()
    : outFrequency(48000)
{
    SDL_Init(SDL_INIT_AUDIO);
    setVolume(255);
}

VM_Host::AudioSettings::~AudioSettings() {
    if (device) SDL_CloseAudioDevice(device);
}

void VM_Host::AudioSettings::setSpec(VM_Host* parent) {
    SDL_zero(spec);
    spec.freq     = outFrequency;
    spec.format   = AUDIO_S16SYS;
    spec.channels = 1;
    spec.samples  = 128;
    spec.callback = &audioCallback;
    spec.userdata = parent;
    device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
}

void VM_Host::AudioSettings::setVolume(const s32 vol) {
    volume = std::clamp(vol, 0, 255);
    amplitude = as<s16>(16 * volume);
}

void VM_Host::AudioSettings::audioCallback(void* data, u8* buffer, const s32 bytes) {
    auto* self = to<VM_Host*>(data);
    if (self->Audio.handler)
        self->Audio.handler(to<s16*>(buffer), bytes >> 1);
}
