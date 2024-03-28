/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cmath>
#include <algorithm>

#include "SoundCores.hpp"
#include "Guest.hpp"
#include "../HostClass/BasicAudioSpec.hpp"


/*------------------------------------------------------------------*/
/*  class  SoundCores                                               */
/*------------------------------------------------------------------*/

SoundCores::SoundCores(VM_Guest& parent, BasicAudioSpec& bas)
    : vm(parent)
    , BAS(bas)
{}

void SoundCores::renderAudio(int16_t* samples, int32_t frames) {
    if (beepFx0A) goto beepFx0A;

    if (MC.isOn()) {
        MC.render(samples, frames);
        return;
    }

    if (!vm.Program.Timer.sound) {
        wavePhase = 0.0f;
        while (frames--)
            *samples++ = 0;
        return;
    }

    if (XO.isOn()) {
        XO.render(samples, frames);
        return;
    } else {
    beepFx0A:
        C8.render(samples, frames);
        return;
    }
}

/*------------------------------------------------------------------*/
/*  class  SoundCores::Classic                                      */
/*------------------------------------------------------------------*/

SoundCores::Classic::Classic(SoundCores& parent, BasicAudioSpec& bas)
    : Sound(parent)
    , BAS(bas)
{}

void SoundCores::Classic::setTone(const std::size_t sp, const std::size_t pc) {
    // sets a unique tone for each sound call
    tone.store((160.0f + 8.0f * ((pc >> 1) + sp + 1 & 0x3E)) / BAS.outFrequency);
}

void SoundCores::Classic::setTone(const std::size_t vx) {
    // sets the tone for each 8X sound call
    tone.store((160.0f + (vx >> 3 << 4)) / BAS.outFrequency);
}

void SoundCores::Classic::render(int16_t* samples, int32_t frames) {
    while (frames--) {
        *samples++ = Sound.wavePhase > 0.5f ? BAS.amplitude : -BAS.amplitude;
        Sound.wavePhase = std::fmod(Sound.wavePhase + tone.load(), 1.0f);
    }
}

/*------------------------------------------------------------------*/
/*  class  SoundCores::XOchip                                       */
/*------------------------------------------------------------------*/

SoundCores::XOchip::XOchip(SoundCores& parent, BasicAudioSpec& bas, VM_Guest& guest)
    : Sound(parent)
    , BAS(bas)
    , vm(guest)
    , rate(4000.0f / 128.0f / BAS.outFrequency)
    , tone(rate)
{}

bool SoundCores::XOchip::isOn() const {
    return enabled;
}

void SoundCores::XOchip::setPitch(const std::size_t pitch) {
    tone.store(rate * std::pow(2.0f, (pitch - 64.0f) / 48.0f));
    enabled = true;
}

void SoundCores::XOchip::loadPattern(std::size_t idx) {
    for (auto& byte : pattern) {
        byte.store(vm.mrw(idx++));

        if (!enabled && byte > 0x0 && byte < 0xFF)
            enabled = true;
    }
}

void SoundCores::XOchip::render(int16_t* samples, int32_t frames) {
    while (frames--) {
        const auto step{ static_cast<int32_t>(std::clamp(Sound.wavePhase * 128.0f, 0.0f, 127.0f)) };
        const auto mask{ 1 << (7 - (step & 7)) };
        *samples++ = pattern[step >> 3].load() & mask ? BAS.amplitude : -BAS.amplitude;
        Sound.wavePhase = std::fmod(Sound.wavePhase + tone.load(), 1.0f);
    }
}

/*------------------------------------------------------------------*/
/*  class  SoundCores::MegaChip                                     */
/*------------------------------------------------------------------*/

SoundCores::MegaChip::MegaChip(SoundCores& parent, BasicAudioSpec& bas, VM_Guest& guest)
    : Sound(parent)
    , BAS(bas)
    , vm(guest)
{}

bool SoundCores::MegaChip::isOn() const {
    return enabled;
}

void SoundCores::MegaChip::reset() {
    enabled = false;
    looping = false;

    length.store(0);
    start.store(0);

    step.store(0.0);
    pos.store(0.0);
}

void SoundCores::MegaChip::enable(
    const std::size_t freq,
    const std::size_t len,
    const std::size_t offset,
    const bool loop
) {
    enabled = true;
    looping = loop;

    start.store(offset);
    step.store(freq * 1.0 / BAS.outFrequency);
    length.store(len);
    pos.store(0.0);
}

void SoundCores::MegaChip::render(int16_t* samples, int32_t frames) {
    while (frames--) {
        auto   _curidx{ vm.mrw(start.load() + static_cast<uint32_t>(pos.load()))};
        double _offset{ pos.load() + step.load()};

        if (_offset >= length.load()) {
            if (looping) {
                _offset -= length.load();
            } else {
                _offset = 0.0;
                _curidx = 128;
                length.store(0);
                enabled = false;
            }
        }
        pos.store(_offset);
        *samples++ = static_cast<int16_t>((_curidx - 128) * BAS.volume);
    }
}
