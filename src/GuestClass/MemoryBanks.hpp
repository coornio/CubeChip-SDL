/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <array>
#include <vector>
#include <cstddef>

#include "Enums.hpp"

template <typename T>
using vec2D = std::vector<std::vector<T>>;

template <typename T, auto X, auto Y = X>
using arr2D = std::array<std::array<T, X>, Y>;

class VM_Guest;

class MemoryBanks final {
    VM_Guest* vm;
    void (*applyViewportMask)(uint32_t&, std::size_t) {};

public:
    arr2D<uint32_t, 256, 192> display{};
    arr2D<uint32_t,  16, 128> bufColor8x{};
    arr2D<uint32_t, 256, 192> bufColorMC{};
    arr2D<uint8_t,  256, 192> bufPalette{};

    std::vector<uint8_t>      ram{};
    std::array<uint32_t, 256> palette{};

    explicit MemoryBanks(VM_Guest*);
    void modifyViewport(BrushType);
    void changeViewportMask(BrushType);

    void flushBuffers(bool);
    void loadPalette(std::size_t, std::size_t);

    void clearPages(std::size_t);
};
