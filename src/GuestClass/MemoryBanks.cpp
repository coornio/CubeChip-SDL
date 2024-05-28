/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <utility>

#include "Guest.hpp"
#include "MemoryBanks.hpp"

MemoryBanks::MemoryBanks(VM_Guest* parent)
	: vm{ parent }
{}

void MemoryBanks::modifyViewport(const BrushType type) {
	vm->isDisplayReady(true);

	switch (type) {

		case BrushType::CLR:
			display.wipeAll();
			return;

		case BrushType::XOR:
			for (auto& row : display)
				row ^= vm->Plane.mask;
			return;

		case BrushType::SUB:
			for (auto& row : display)
				row &= ~vm->Plane.mask;
			return;

		case BrushType::ADD:
			for (auto& row : display)
				row |= vm->Plane.mask;
			return;
	}
}

void MemoryBanks::flushBuffers(const bool firstFlush) {
	vm->isDisplayReady(true);

	if (firstFlush) palette.fill(0);
	else display.copyLinear(bufColorMC);

	bufColorMC.wipeAll();
	bufPalette.wipeAll();
}

void MemoryBanks::loadPalette(std::int32_t index, const std::int32_t count) {
	for (std::size_t idx{ 0 }; std::cmp_less(idx, count); index += 4) {
		palette[++idx] = static_cast<std::uint32_t>(
			vm->mrw(index + 0u) << 24u |
			vm->mrw(index + 1u) << 16u |
			vm->mrw(index + 2u) <<  8u |
			vm->mrw(index + 3u)
		);
	}
}

void MemoryBanks::clearPages(std::int32_t H) {
	vm->isDisplayReady(true);

	while (H++ < vm->Plane.H)
		display[H].wipeAll();
}
