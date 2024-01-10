
#include "Guest.hpp"

/*------------------------------------------------------------------*/
/*  struct  VM_Guest::MemoryBanks                                   */
/*------------------------------------------------------------------*/

VM_Guest::MemoryBanks::MemoryBanks(VM_Guest& parent) : vm(parent) {}

void VM_Guest::MemoryBanks::changeViewportMask(const BrushType type) {
    switch (type) {

        case BrushType::CLR:
            applyViewportMask = [](u32& pos, const u32) { pos = 0; };
            return;

        case BrushType::XOR:
            applyViewportMask = [](u32& pos, const u32 mask) { pos ^= mask; };
            return;

        case BrushType::SUB:
            applyViewportMask = [](u32& pos, const u32 mask) { pos &= ~mask; };
            return;

        case BrushType::ADD:
            applyViewportMask = [](u32& pos, const u32 mask) { pos |= mask; };
            return;
    }
}

void VM_Guest::MemoryBanks::modifyViewport(const BrushType type) {
    vm.State.push_display = true;
    changeViewportMask(type);

    for (auto H{ 0 }; H < vm.Plane.H; ++H)
    for (auto X{ 0 }; X < vm.Plane.X; ++X)
        applyViewportMask(display[H][X], vm.Plane.mask);
}

void VM_Guest::MemoryBanks::flushBuffers(const bool firstFlush) {
    vm.State.push_display = true;

    if (firstFlush) palette.fill(0);
    else display = bufColorMC;

    for (auto& row : bufColorMC) row.fill(0);
    for (auto& row : bufPalette) row.fill(0);
}

void VM_Guest::MemoryBanks::loadPalette(u32 index, const u8 count) {
    for (auto idx{ 0 }; idx < count;) {
        palette[++idx] =
            vm.mrw(index++) << 24 |
            vm.mrw(index++) << 16 |
            vm.mrw(index++) << 8 |
            vm.mrw(index++);
    }
}

void VM_Guest::MemoryBanks::clearPages(s32 H) {
    vm.State.push_display = true;

    while (H++ < vm.Plane.H)
        for (auto X{ 0 }; X < vm.Plane.X; ++X)
            display[H][X] = 0;
}
