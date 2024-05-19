/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Guest.hpp"
#include "MemoryBanks.hpp"

MemoryBanks::MemoryBanks(VM_Guest* parent)
	: vm{ parent }
{}

void MemoryBanks::changeViewportMask(const BrushType type) {
	switch (type) {

		case BrushType::CLR:
			applyViewportMask = [](std::uint32_t& pos, const std::uint32_t) { pos = 0; };
			return;

		case BrushType::XOR:
			applyViewportMask = [](std::uint32_t& pos, const std::uint32_t mask) { pos ^= mask; };
			return;

		case BrushType::SUB:
			applyViewportMask = [](std::uint32_t& pos, const std::uint32_t mask) { pos &= ~mask; };
			return;

		case BrushType::ADD:
			applyViewportMask = [](std::uint32_t& pos, const std::uint32_t mask) { pos |= mask; };
			return;
	}
}

void MemoryBanks::modifyViewport(const BrushType type) {
	vm->State.push_display = true;
	changeViewportMask(type);

	for (auto& row : display)
		for (auto& elem : row)
			applyViewportMask(elem, vm->Plane.mask);
}

void MemoryBanks::flushBuffers(const bool firstFlush) {
	vm->State.push_display = true;

	if (firstFlush) palette.fill(0);
	else display.copyLinear(bufColorMC);

	bufColorMC.wipeAll();
	bufPalette.wipeAll();
}

void MemoryBanks::loadPalette(std::int32_t index, const std::int32_t count) {
	for (auto idx{ 0 }; idx < count; index += 4) {
		palette[++idx] =
			vm->mrw(index + 0) << 24 |
			vm->mrw(index + 1) << 16 |
			vm->mrw(index + 2) << 8 |
			vm->mrw(index + 3);
	}
}

void MemoryBanks::clearPages(std::int32_t H) {
	vm->State.push_display = true;

	while (H++ < vm->Plane.H)
		display[H].wipeAll();
}
