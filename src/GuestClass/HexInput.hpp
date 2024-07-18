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

	      std::vector<KeyInfo> currentBinds;
	const std::vector<KeyInfo> defaultBinds;

	Uint32 keysCurr{}; // bitfield of key states in current frame
	Uint32 keysPrev{}; // bitfield of key states in previous frame
	Uint32 keysLock{}; // bitfield of keys excluded from input checks

public:
	explicit HexInput();

	void loadPresetBinds();
	void loadCustomBinds(const std::vector<KeyInfo>& bindings);

	void clearLockStates();
	void updateKeyStates();

	bool keyPressed(Uint8& ret);
	bool keyPressed(std::size_t index, std::size_t offset) const;
};
