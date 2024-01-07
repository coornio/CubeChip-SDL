
#include "Guest.hpp"
#include "../HostClass/Host.hpp"

/*------------------------------------------------------------------*/
/*  class  VM_Guest::AudioCores                                     */
/*------------------------------------------------------------------*/

VM_Guest::AudioCores::AudioCores(VM_Guest& parent)
    : vm(parent)
    , outFreq(vm.Host.Audio.outFrequency)
    , volume(vm.Host.Audio.volume)
{}

void VM_Guest::AudioCores::initializeCores() {
    wavePhase = 0.0f;
    C8.reset();
    XO.reset();
    MC.reset();
}

void VM_Guest::AudioCores::renderAudio(s16* samples, size_t frames) {
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
    }
    else {
    beepFx0A:
        C8.render(samples, frames);
        return;
    }

blank:
    wavePhase = 0.0f;
    while (frames--)
        *samples++ = 0;
}

/*------------------------------------------------------------------*/
/*  class  VM_Guest::AudioCores::Classic                            */
/*------------------------------------------------------------------*/

VM_Guest::AudioCores::Classic::Classic(AudioCores& parent) : Audio(parent) {}

void VM_Guest::AudioCores::Classic::reset() {
    beepFx0A = false;
    tone.store(500.0f);
}

void VM_Guest::AudioCores::Classic::render(s16* samples, size_t frames) {
    const auto amplitude{ as<s16>(32767.0f * pow(10.0f, (1.0f - Audio.volume * 0.90f) * -96.0f / 20.0f)) };
    const auto step{ tone / Audio.outFreq };

    while (frames--) {
        *samples++ = (Audio.wavePhase > 0.5f) ? amplitude : -amplitude;
        Audio.wavePhase = std::fmod(Audio.wavePhase + step, 1.0f);
    }
}

/*------------------------------------------------------------------*/
/*  class  VM_Guest::AudioCores::XOchip                             */
/*------------------------------------------------------------------*/

VM_Guest::AudioCores::XOchip::XOchip(AudioCores& parent) : Audio(parent) {}

void VM_Guest::AudioCores::XOchip::reset() {
    enabled = false;
    pitch.store(64);
}

void VM_Guest::AudioCores::XOchip::loadPattern(u32 idx) {
    for (auto& byte : pattern) {
        byte = Audio.vm.mrw(idx++);

        if (byte > 0x0 && byte < 0xFF)
            enabled = true;
    }
}

void VM_Guest::AudioCores::XOchip::render(s16* samples, size_t frames) {
    const auto amplitude{ as<s16>(32767.0f * pow(10.0f, (1.0f - Audio.volume * 0.90f) * -96.0f / 20.0f)) };
    const auto step{ 4000 * std::pow(2.0f, (pitch - 64.0f) / 48.0f) / 128 / Audio.outFreq };

    while (frames--) {
        const auto pos{ as<u8>(std::clamp(Audio.wavePhase * 128.0f, 0.0f, 127.0f)) };
        *samples++ = pattern[pos >> 3] & (1 << (7 - (pos & 7))) ? amplitude : -amplitude;
        Audio.wavePhase = std::fmod(Audio.wavePhase + step, 1.0f);
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
    while (frames--) {
        u8     _val{ Audio.vm.mrw(start + as<u32>(pos)) };
        double _bit{ pos + step };

        if (_bit >= length) {
            if (looping)
                _bit -= as<double>(length);
            else {
                _bit = 0.0;
                length.store(0);
                _val = 128;
            }
        }
        pos.store(_bit);
        *samples++ = as<u16>(_val - 128) * 256;
    }
}
