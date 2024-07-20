/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <SDL3/SDL_scancode.h>
#include <vector>

class HexInput final {
	struct KeyInfo {
		Uint32       idx; // key index on chip8 pad
		SDL_Scancode key; // main keyboard equivalent
		SDL_Scancode alt; // alternative option
	};

	static constexpr auto _{ SDL_SCANCODE_UNKNOWN };

	std::vector<KeyInfo> mCustomBinds;
	std::vector<KeyInfo> mPresetBinds;

	Uint32 mTickLast{};
	Uint32 mTickRate{};

	Uint32 mKeysCurr{}; // bitfield of key states in current frame
	Uint32 mKeysPrev{}; // bitfield of key states in previous frame
	Uint32 mKeysLock{}; // bitfield of keys excluded from input checks
	Uint32 mKeysLoop{}; // bitfield of keys repeating input on Fx0A

public:
	explicit HexInput();

	void loadPresetBinds();
	void loadCustomBinds(const std::vector<KeyInfo>& bindings);

	void updateKeyStates(Uint32 counter);

	bool keyPressed(Uint8* returnKey);
	bool keyPressed(std::size_t index, std::size_t offset) const;
};
