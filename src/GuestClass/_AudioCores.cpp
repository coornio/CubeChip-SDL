/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Guest.hpp"
#include "../HostClass/Host.hpp"

/*------------------------------------------------------------------*/
/*  class  VM_Guest::AudioCores                                     */
/*------------------------------------------------------------------*/

VM_Guest::AudioCores::AudioCores(VM_Guest& parent)
    : vm(parent)
    , outFreq(vm.Host.Audio.outFrequency)
    , volume(vm.Host.Audio.volumeLog)
    , wavePhase(0.0f)
{}

void VM_Guest::AudioCores::renderAudio(s16* samples, u32 frames) {
    if (C8.beepFx0A) goto beepFx0A;

    if (MC.enabled) {
        if (!MC.length.load()) goto blank;
        MC.render(samples, frames);
        return;
    }

    if (!vm.Program.Timer.sound) goto blank;

    if (XO.enabled) {
        XO.render(samples, frames);
        return;
    } else {
    beepFx0A:
        C8.render(samples, frames);
        return;
    }

blank:
    wavePhase = 0.0f;
    while (frames--)
        *samples++ = 0;
}

void VM_Guest::AudioCores::modifyAmp() {
    amplitude = as<u16>(32767.0f * volume * 0.2f);
}

/*------------------------------------------------------------------*/
/*  class  VM_Guest::AudioCores::Classic                            */
/*------------------------------------------------------------------*/

VM_Guest::AudioCores::Classic::Classic(AudioCores& parent) : Audio(parent) {}

void VM_Guest::AudioCores::Classic::setTone(const u8 sp, const u32 pc) {
    // sets a unique tone for each sound call
    tone.store((160.0f + 8.0f * ((pc >> 1) + sp + 1 & 0x3E)) / Audio.outFreq);
}

void VM_Guest::AudioCores::Classic::setTone(const u8 vx) {
    // sets the tone for each 8X sound call
    tone.store((160.0f + (vx >> 3 << 4)) / Audio.outFreq);
}

void VM_Guest::AudioCores::Classic::render(s16* samples, size_t frames) {
    while (frames--) {
        *samples++ = Audio.wavePhase > 0.5f ? Audio.amplitude : -Audio.amplitude;
        Audio.wavePhase = std::fmod(Audio.wavePhase + tone.load(), 1.0f);
    }
}

/*------------------------------------------------------------------*/
/*  class  VM_Guest::AudioCores::XOchip                             */
/*------------------------------------------------------------------*/

VM_Guest::AudioCores::XOchip::XOchip(AudioCores& parent) : Audio(parent) {
    setPitch(64);
    enabled = false;
}

void VM_Guest::AudioCores::XOchip::setPitch(u8 pitch) {
    tone.store(
        4000.0f * std::pow(2.0f, (pitch - 64.0f) / 48.0f) \
        / 128.0f / Audio.outFreq
    );
    enabled = true;
}

void VM_Guest::AudioCores::XOchip::loadPattern(u32 idx) {
    for (auto& byte : pattern) {
        byte = Audio.vm.mrw(idx++);

        if (byte > 0x0 && byte < 0xFF)
            enabled = true;
    }
}

void VM_Guest::AudioCores::XOchip::render(s16* samples, size_t frames) {
    while (frames--) {
        const auto pos{ as<u8>(std::clamp(Audio.wavePhase * 128.0f, 0.0f, 127.0f)) };
        *samples++ = pattern[pos >> 3] & (1 << (7 - (pos & 7))) ? Audio.amplitude : -Audio.amplitude;
        Audio.wavePhase = std::fmod(Audio.wavePhase + tone, 1.0f);
    }
}

/*------------------------------------------------------------------*/
/*  class  VM_Guest::AudioCores::MegaChip                           */
/*------------------------------------------------------------------*/

VM_Guest::AudioCores::MegaChip::MegaChip(AudioCores& parent) : Audio(parent) {}

void VM_Guest::AudioCores::MegaChip::reset() {
    enabled = false;
    looping = false;

    length.store(0);
    start.store(0);

    step.store(0.0);
    pos.store(0.0);
}

void VM_Guest::AudioCores::MegaChip::enable(
    const u32 freq,
    const u32 len,
    const u32 offset,
    const bool loop
) {
    enabled = true;
    looping = loop;

    start.store(offset);
    step.store(freq * 1.0 / Audio.outFreq);
    length.store(len);
    pos.store(0.0);
}

void VM_Guest::AudioCores::MegaChip::render(s16* samples, size_t frames) {
    const auto _volume{ Audio.volume * 256.0f };
    while (frames--) {
        auto   _curidx{ Audio.vm.mrw(start.load() + as<u32>(pos.load()))};
        double _offset{ pos.load() + step.load()};

        if (_offset >= length.load()) {
            if (looping) {
                _offset -= length.load();
            } else {
                _offset = 0.0;
                _curidx = 128;
                length.store(0);
            }
        }
        pos.store(_offset);
        *samples++ = as<u16>((_curidx - 128) * _volume);
    }
}
