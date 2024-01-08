
#include "Host.hpp"

/*------------------------------------------------------------------*/
/*  class  VM_Host::AudioSettings                                   */
/*------------------------------------------------------------------*/

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

void VM_Host::AudioSettings::setVolume(const float vol) {
    volume = std::clamp(vol, 0.0f, 1.0f);
}

void VM_Host::AudioSettings::audioCallback(void* data, u8* buffer, const s32 bytes) {
    auto* self = reinterpret_cast<VM_Host*>(data);
    if (self->Audio.handler)
        self->Audio.handler(reinterpret_cast<s16*>(buffer), bytes >> 1);
}
