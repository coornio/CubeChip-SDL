/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Guest.hpp"
#include "MemoryBanks.hpp"

MemoryBanks::MemoryBanks(VM_Guest* parent)
    : vm(parent)
{}

void MemoryBanks::changeViewportMask(const BrushType type) {
    switch (type) {

        case BrushType::CLR:
            applyViewportMask = [](uint32_t& pos, const std::size_t) { pos = 0; };
            return;

        case BrushType::XOR:
            applyViewportMask = [](uint32_t& pos, const std::size_t mask) { pos ^= mask; };
            return;

        case BrushType::SUB:
            applyViewportMask = [](uint32_t& pos, const std::size_t mask) { pos &= ~mask; };
            return;

        case BrushType::ADD:
            applyViewportMask = [](uint32_t& pos, const std::size_t mask) { pos |= mask; };
            return;
    }
}

void MemoryBanks::modifyViewport(const BrushType type) {
    vm->State.push_display = true;
    changeViewportMask(type);

    for (auto H{ 0 }; H < vm->Plane.H; ++H)
    for (auto X{ 0 }; X < vm->Plane.X; ++X)
        applyViewportMask(display[H][X], vm->Plane.mask);
}

void MemoryBanks::flushBuffers(const bool firstFlush) {
    vm->State.push_display = true;

    if (firstFlush) palette.fill(0);
    else display = bufColorMC;

    for (auto& row : bufColorMC) row.fill(0);
    for (auto& row : bufPalette) row.fill(0);
}

void MemoryBanks::loadPalette(std::size_t index, const std::size_t count) {
    for (auto idx{ 0 }; idx < count; index += 4) {
        palette[++idx] =
            vm->mrw(index + 0) << 24 |
            vm->mrw(index + 1) << 16 |
            vm->mrw(index + 2) <<  8 |
            vm->mrw(index + 3);
    }
}

void MemoryBanks::clearPages(std::size_t H) {
    vm->State.push_display = true;

    while (H++ < vm->Plane.H)
        for (auto X{ 0 }; X < vm->Plane.X; ++X)
            display[H][X] = 0;
}
