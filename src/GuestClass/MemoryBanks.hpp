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

class VM_Guest;

class MemoryBanks final {
	VM_Guest* vm;

public:
	std::vector<std::uint8_t>  memory{};
	std::vector<std::uint32_t> megaPalette{};

	Map2D<std::uint32_t> foregroundBuffer;
	Map2D<std::uint32_t> backgroundBuffer;
	Map2D<std::uint8_t>  collisionPalette;

	Map2D<std::uint8_t>  displayBuffer[4];
	Map2D<std::uint32_t> color8xBuffer;

	explicit MemoryBanks(VM_Guest*);
	void modifyViewport(BrushType);

	void flushBuffers(bool);
	void loadPalette(std::int32_t, std::int32_t);

	void clearPages(std::int32_t);
};
