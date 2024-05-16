/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <array>
#include <vector>
#include <cstddef>

#include "../Assistants/Map2D.hpp"
#include "Enums.hpp"

//template <typename T>
//using vec2D = std::vector<std::vector<T>>;

//template <typename T, auto X, auto Y = X>
//using arr2D = std::array<std::array<T, X>, Y>;

class VM_Guest;

class MemoryBanks final {
    VM_Guest* vm;
    void (*applyViewportMask)(std::uint32_t&, std::uint32_t) {};

public:
    Map2D<std::uint32_t> display;
    Map2D<std::uint32_t> bufColor8x;
    Map2D<std::uint32_t> bufColorMC;
    Map2D<std::uint8_t>  bufPalette;
    //arr2D<uint32_t, 256, 192> display{};
    //arr2D<uint32_t,  16, 128> bufColor8x{};
    //arr2D<uint32_t, 256, 192> bufColorMC{};
    //arr2D<uint8_t,  256, 192> bufPalette{};

    std::vector<std::uint8_t>      ram{};
    std::array<std::uint32_t, 256> palette{};

    explicit MemoryBanks(VM_Guest*);
    void modifyViewport(BrushType);
    void changeViewportMask(BrushType);

    void flushBuffers(bool);
    void loadPalette(std::int32_t, std::int32_t);

    void clearPages(std::int32_t);
};
