
#include "Guest.hpp"

/*------------------------------------------------------------------*/
/*  struct  VM_Guest::MemoryBanks                                   */
/*------------------------------------------------------------------*/

VM_Guest::MemoryBanks::MemoryBanks(VM_Guest& parent) : vm(parent) {}

void VM_Guest::MemoryBanks::changeViewportMask(BrushType type) {
    switch (type) {

        case BrushType::CLR:
            applyViewportMask = [](u32& pos, u32) { pos = 0; };
            return;

        case BrushType::XOR:
            applyViewportMask = [](u32& pos, u32 mask) { pos ^= mask; };
            return;

        case BrushType::SUB:
            applyViewportMask = [](u32& pos, u32 mask) { pos &= ~mask; };
            return;

        case BrushType::ADD:
            applyViewportMask = [](u32& pos, u32 mask) { pos |= mask; };
            return;
    }
}

/*
void VM_Guest::MemoryBanks::resizeViewportBuffers() {
    if (vm.State.xochip_color) {
        resize2Dvec(display, vm.Plane.X);
        return;
    }
    static constexpr std::array<std::string, 1> var = { "spam" };
    if (vm.State.mega_enabled) {
        resize2Dvec(display, vm.Plane.W);
        resize2Dvec(bufColorMC, vm.Plane.W);
        resize2Dvec(bufPalette, vm.Plane.W);
        return;
    } else {
        resize2Dvec(display, vm.Plane.X);
        if (vm.State.chip8X_rom)
            resize2Dvec(bufColor8x, vm.Plane.X);
    }
}
*/

void VM_Guest::MemoryBanks::modifyViewport(BrushType type) {
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

void VM_Guest::MemoryBanks::loadPalette(u32 index, u8 count) {
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
