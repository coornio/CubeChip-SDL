# CubeChip (SDL)

An emulator written in C++ and SDL (currently still a WIP) that I started as an effort to learn the language, albeit belatedly. Aiming to be as accurate as possible from an HLE standpoint, while also aiming to be fairly fast on its feet. Input, audio and video are handled through SDL2, and the plan is to use ImGUI later to get an actual interface going, as everything is currently more or less hardcoded and inflexible.

## Supported Platforms

Currently supports the following major platforms:
- CHIP-8[^1]
- SUPERCHIP[^2]
- XOCHIP[^3]
- MEGACHIP[^4]

[^1]: Also supports legacy behavior mode (planned to be cycle-accurate).
[^2]: Also supports legacy behavior mode (seen in HP48 graphing calculators).
[^3]: Also supports 4-planes rendering, and an instruction declined from the official spec.
[^4]: Potentially officially the first emulator since Mega8 to [run the respective demo properly](https://www.youtube.com/watch?v=Z215BO9Gkko).

The following platform extensions are available, though specific combinations aren't possible (will later be detailed in a table):
- HIRES MPD[^5]
- CHIP-8E[^6]
- CHIP-8X[^7]
- HWCHIP64[^8]
- GIGACHIP[^9]

[^5]: Stands for Multi-Page-Display. Runs both original 2-page and 4-page roms, as well as patched roms.
[^6]: Exclusive mod to CHIP-8 and SUPERCHIP. Does not work with HIRES MPD. Mutually exclusive with CHIP-8X.
[^7]: Exclusive mod to CHIP-8 and SUPERCHIP. Works with HIRES MPD. Mutually exclusive with CHIP-8E.
[^8]: Extension to XOCHIP, mutually exclusive with all aforementioned extensions.
[^9]: Extension to MEGACHIP, though incompatible with it. Offers tons of new texture render features. (WIP).

## Planned Features

[ ] Implement SHA1 encoding to compare roms and later hook up with a database for settings.
[ ] Implement nlohmann's JSON class so that I can handle settings files and databases.
[ ] Implement ImGUI to get a rudimentary and reactive interface going instead of the current single render window.
[ ] Refactor the main loop logic so the SDL event loop isn't running only when a valid rom is present.
[ ] Far future: refactor the class system to move away from the monolithic class system in favor of a component system as described in [this article](http://gameprogrammingpatterns.com/component.html).
[ ] Refactor the guest class and separate the instruction tree into different platform variants for cleaner, more streamlined code with less branching.
