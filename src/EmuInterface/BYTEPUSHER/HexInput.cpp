/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <bit>

#include "../../Assistants/BasicInput.hpp"

#include "HexInput.hpp"

void HexInput::loadPresetBinds() {
	static constexpr auto _{ SDL_SCANCODE_UNKNOWN };
	loadCustomBinds({
		{0x1, KEY(1), _}, {0x2, KEY(2), _}, {0x3, KEY(3), _}, {0xC, KEY(4), _},
		{0x4, KEY(Q), _}, {0x5, KEY(W), _}, {0x6, KEY(E), _}, {0xD, KEY(R), _},
		{0x7, KEY(A), _}, {0x8, KEY(S), _}, {0x9, KEY(D), _}, {0xE, KEY(F), _},
		{0xA, KEY(Z), _}, {0x0, KEY(X), _}, {0xB, KEY(C), _}, {0xF, KEY(V), _},
	});
}

void HexInput::loadCustomBinds(std::vector<KeyInfo>&& binds) {
	(mCustomBinds = binds).resize(binds.size());
}

Uint32 HexInput::getKeyStates() const {
	Uint32 keyStates{};

	for (const auto& mapping : mCustomBinds) {
		if (binput::kb.areAnyHeld(mapping.key, mapping.alt)) {
			keyStates |= 1 << mapping.idx;
		}
	}

	return keyStates;
}
