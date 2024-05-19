/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <bit>

#include "../Assistants/BasicInput.hpp"

#include "HexInput.hpp"

HexInput::HexInput()
	: currentBinds(32)
	, defaultBinds{
		{0x1, KEY(1), _}, {0x2, KEY(2), _}, {0x3, KEY(3), _}, {0xC, KEY(4), _},
		{0x4, KEY(Q), _}, {0x5, KEY(W), _}, {0x6, KEY(E), _}, {0xD, KEY(R), _},
		{0x7, KEY(A), _}, {0x8, KEY(S), _}, {0x9, KEY(D), _}, {0xE, KEY(F), _},
		{0xA, KEY(Z), _}, {0x0, KEY(X), _}, {0xB, KEY(C), _}, {0xF, KEY(V), _},
	} {}

void HexInput::reset() {
	setup(defaultBinds);
}

void HexInput::refresh() {
	if (!currentBinds.size()) return;

	keysPrev = keysCurr;
	keysCurr = 0u;

	for (const auto& mapping : currentBinds)
		if (bic::kb.areAnyHeld(mapping.key, mapping.alt))
			keysCurr |= 1u << mapping.idx;
	keysLock &= ~(keysPrev ^ keysCurr);
}

void HexInput::setup(const std::vector<KeyInfo>& bindings) {
	(currentBinds = bindings).resize(bindings.size());
	keysPrev = keysCurr = keysLock = 0u;
}

bool HexInput::keyPressed(Uint8& ret) {
	if (!currentBinds.size()) return false;

	const auto mask{ keysCurr & ~keysPrev & ~keysLock };
	if (mask) {
		ret = static_cast<Uint8>
			(std::countr_zero(mask & ~(mask - 1u)));
		keysLock |= mask;
		return true;
	}
	return false;
}

bool HexInput::keyPressed(const std::size_t index, const std::size_t offset) const {
	return keysCurr & ~keysLock & 1u << ((index & 0xFu) + offset);
}

Uint32 HexInput::currKeys(const std::size_t index) const {
	return keysLock >> index & 0x1u;
}
