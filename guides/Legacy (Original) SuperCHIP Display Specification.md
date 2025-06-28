# Legacy (Original) SuperCHIP Display Specification

This is a quick specification and guide for the aforementioned platform's `DxyN` instruction. It will go into details about the requirements and restrictions of both in order for one to implement them accurately and match the behavior of hardware as observed on calculators running the original SuperCHIP rom such as the **HP 48SX**, among others.

## Screen Properties
- The screen (and timers too) refreshes at a rate of 64hz.
- It has a fixed resolution of 128 × 64 pixels.

I should note here that, unlike the Cosmac VIP which could run a variable amount of instructions in a frame before it had to interrupt for vblank, the calculators that "succeeded" it through SuperCHIP were several times faster. Thus, the vblank is emulated naively only during `DxyN` instructions, and only when not in **Extended Mode**.

- It emulates a "vblank" period for each `DxyN` call when not in Extended Mode. This, effectively, ensures only a single `DxyN` instruction can run in a frame, unless Extended Mode is enabled.
- The `00FE` and `00FF` instructions do not actually change the screen resolution, but merely control whether Extended Mode is disabled/enabled respectively.

### The heck is "Extended Mode" ?
It might sound like something peculiar, but in practice it is merely a boolean setting which influences how the `DxyN` instruction operates. The important bit however is the system always boots with it disabled. *The related instructions do **not** act as toggles.*

### So how does it affect drawing with `DxyN` ?
There's actually a fair amount of differences involved! No joke. The `DxyN` instruction already has differences based on whether the last (`N`) nibble is a `0` or not, but Extended Mode influences those two variations further, effectively resulting in 4 branches. Feeling annoyed yet? 

Let's get right into the gritty of it, I'll explain the differences between `Dxy0` and `DxyN` in two separate segments, depending on whether Extended Mode is enabled or not.

I should also preface the upcoming segments with a note about whether sprites drawn must wrap around the edges or not -- by default they should not, and any "runoff" should be discarded. If you wish though, you may allow wrapping by use of some toggle. It will complicate your work a bit though.

## Extended Mode: **ON**
### Instruction: `Dxy0` --- 16 × 16 Sprites
1) Copy `V[x]` and `V[y]` coordinates first, and wrap the values to the size of the respective screen axis size. E.g. if `V[x]` is `132`, you would mask with `0x7F` (`width - 1`) and get `4` back.
2) Loop for 16 rows. For each row, you will be fetching two bytes from `memory` at `I + row * 2` with `+ 0` and `+ 1` respectively for the two bytes.
3) Merge the two bytes into a single value by bit-shifting the first one left by `8` positions. This will result in a value that is 16-bit, thus `16` pixels.
4) Loop for `16` columns. For each column, you will be fetching the relevant bit from the previously calculated you got when combining the bytes (starting from the most significant bit). If the bit is a `0`, you may skip to the next.
5) When a bit is `1`, you can xor against the pixel of your `display` array at the same position (derived from the copied coordinates from step **#1**, adding the `row` and `column` accordingly). If the display's pixel was turned off as a result, that counts as a collision.
6) At the end of the loops, set `V[0xF]` to however many **rows** had a collision occur.

### Instruction: `DxyN` --- 8 × N Sprites
1) Copy `V[x]` and `V[y]` coordinates first, and wrap the values to the size of the respective screen axis size. E.g. if `V[x]` is `132`, you would would mask with `0x7F` (`width - 1`) and get `4` back.
2) Loop for `N` rows. For each row, you will be fetching one byte from `memory` at `I + row`.
3) Loop for `8` columns. For each column, you will be fetching the relevant bit from the previously calculated (starting from the most significant bit). If the bit is a `0`, you may skip to the next.
4) When a bit is `1`, you can xor against the pixel of your `display` array at the same position (derived from the copied coordinates from step **#1**, adding the `row` and `column` accordingly). If the display's pixel was turned off as a result, that counts as a collision.
5) At the end of the loops, set `V[0xF]` to however many **rows** had a collision occur.

## Extended Mode: **OFF**
### Instruction: `DxyN` --- 8 × N Sprites *(but actually 16 × N\*2)*
This is where most of the complexity lies. This mode "doubles up" draws to make them larger, so a "single" pixel drawn will correspond to a 2 × 2 draw internally to the `display`, faking a 64 × 32 screen to the observer.
This also results in scrolling of the screen being half as effective when Extended Mode is disabled, allowing for the rumored "half-pixel scroll" to take place vertically, and side scrolls only seeming to move two perceived pixels across instead of four.
The problems, instead, begin with how it draws the top half of a pixel and then copies the aftermath to the bottom half. It is unique, and we need to calculate zone offsets. Strap in.

##### Combined with `Dxy0` for convenience, if `N` is `0`, then assume it is `16` instead.

1) First, we must calculate how many positions the `V[x]` coordinate is offset from a 16-pixel wide zone. Calculate by modulo of `8`, then multiply the result by `2` and subtract it from `16`.
2) Copy `V[x]` and `V[y]` coordinates next, and multiply them by `2`. For `V[x]`, you want to mask with `0x70` so that the coordinate wraps around properly, and also snaps to a zone boundary. For `V[y]` you can mask with `0x3E` to wrap around properly and also omit odd positions to be safe.
3) Loop for `N` rows. For each row, you will be fetching one byte from `memory` at `I + row`.
4) Now you must "**bloat**" the byte so that each bit takes up two positions, effectively turning 8-bit to 16-bit. Then, bit-shift it left based on the offset calculated in step **#1**. *The resulting number may be as large as 32-bit, so don't limit your type prematurely.*
5) Loop for `32` columns (you heard *that* right). For each column, you will be fetching the relevant bit from the previously calculated value (starting from the most significant bit). If the bit is a `0`, you **do not skip**.
6) When a bit is `1`, you can xor against the pixel of your `display` array at the same position (derived from the coordinates from step **#2**, adding the `row*2` and `column` accordingly). If the display's pixel was turned off as a result, that counts as a collision.
7) As an extra step, you must now copy the value of the pixel at the previously written-to `display` position, and copy it right below to the next row at `row*2 + 1`. That's right. Xoring and collision occurs **only** for the top row, and the result is duplicated at the bottom half of the perceived 2 × 2 pixel.
8) At the end of the loops, set `V[0xF]` to `1` or `0` depending on whether **any row** had a collision occur.

Hopefully this all made sense. It's very convoluted indeed, but there's definitely worse things to implement out there, like a PPU. Now, perhaps you wondered how to actually pull off the "bloat" I mentioned earlier. You might not have a built-in option to do this, so there's a bit-twiddling snippet of pseudocode you can adopt for your use: 
```cpp
uint32 bitBloat(uint32 value) {
	if (byte == 0) { return 0; }
	byte = (byte << 4 | byte) & 0x0F0F;
	byte = (byte << 2 | byte) & 0x3333;
	byte = (byte << 1 | byte) & 0x5555;
	return  byte << 1 | byte;
}
```