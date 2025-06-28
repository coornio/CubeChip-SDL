# CubeChip (SDL) - WORK IN PROGRESS

An multi-emulator written in C++ and SDL (currently still a WIP) that I started as an effort to learn the language, albeit belatedly. Aiming to be as accurate as possible from an HLE standpoint, while also aiming to be fairly fast on its feet. Input, audio and video are handled through SDL3, with the interface being (slowly) fleshed out with ImGUI.

## Supported Systems

### BytePusher
Merely there for the sake of completion.

### GameBoy
On the roadmap, but hardly anything is implemented yet.

### CHIP-8
Currently supports the following major variants:
- CHIP-8 [^1]
- SUPERCHIP [^2]
- XOCHIP [^3]
- MEGACHIP [^4]

[^1]: Also supports legacy behavior mode (planned to be cycle-accurate).
[^2]: Also supports legacy behavior mode (seen in HP48 graphing calculators).
[^3]: Also supports 4-planes rendering, and an instruction declined from the official spec.
[^4]: Potentially officially the first emulator since Mega8 to [run the respective demo properly](https://www.youtube.com/watch?v=Z215BO9Gkko).

The following platform extensions/mods are available:
- HIRES MPD [^5]
- CHIP-8E [^6]
- CHIP-8X [^7]
- HWCHIP64 [^8]
- GIGACHIP [^9]

Some extension combinations aren't possible, see footnotes for now. Due to major refactoring of the codebase, many variants are currently unimplemented, their code unported and in temporary limbo.

[^5]: Stands for Multi-Page-Display. Runs both original 2-page and 4-page roms, as well as patched roms.
[^6]: Exclusive mod to CHIP-8 and SUPERCHIP. Does not work with HIRES MPD. Mutually exclusive with CHIP-8X.
[^7]: Exclusive mod to CHIP-8 and SUPERCHIP. Works with HIRES MPD. Mutually exclusive with CHIP-8E.
[^8]: Extension to XOCHIP, mutually exclusive with all aforementioned extensions. Designed by [@NinjaWeedle](https://github.com/NinjaWeedle/HyperWaveCHIP-64/tree/master).
[^9]: Derivative of MEGACHIP, though incompatible with it. Offers tons of new texture render features. (WIP). Own design.

## Planned Features

- [x] Implement SHA1 encoding to hash files for identifying different roms.
- [ ] Utilize nlohmann's JSON class so that I can handle settings files and databases.
- [ ] Utilize the SHA1 hash and JSON to hook up with a rom database.
- [x] Implement ImGUI to get a rudimentary and reactive interface going instead of the current single render window.
- [x] Refactor the main loop logic so the SDL event loop isn't running only when a valid rom is present.
- [x] Refactor the guest class and separate the instruction tree into different platform variants for cleaner, more streamlined code with less branching.
- [ ] Find time to focus entirely on writing the Gameboy core.
