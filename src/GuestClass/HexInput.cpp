/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <bit>

#include "../Assistants/BasicInput.hpp"

#include "HexInput.hpp"

HexInput::HexInput()
	: mCustomBinds(32)
	, mPresetBinds{
		{0x1, KEY(1), _}, {0x2, KEY(2), _}, {0x3, KEY(3), _}, {0xC, KEY(4), _},
		{0x4, KEY(Q), _}, {0x5, KEY(W), _}, {0x6, KEY(E), _}, {0xD, KEY(R), _},
		{0x7, KEY(A), _}, {0x8, KEY(S), _}, {0x9, KEY(D), _}, {0xE, KEY(F), _},
		{0xA, KEY(Z), _}, {0x0, KEY(X), _}, {0xB, KEY(C), _}, {0xF, KEY(V), _},
	}
{}

void HexInput::loadPresetBinds() {
	loadCustomBinds(mPresetBinds);
}

void HexInput::loadCustomBinds(const std::vector<KeyInfo>& bindings) {
	(mCustomBinds = bindings).resize(bindings.size());
	mKeysPrev = mKeysCurr = mKeysLock = 0;
}

void HexInput::updateKeyStates() {
	if (!mCustomBinds.size()) { return; }

	mKeysPrev = mKeysCurr;
	mKeysCurr = 0;

	for (const auto& mapping : mCustomBinds) {
		if (bic::kb.areAnyHeld(mapping.key, mapping.alt)) {
			mKeysCurr |= 1 << mapping.idx;
		}
	}

	mKeysLoop &= mKeysLock &= ~(mKeysPrev ^ mKeysCurr);
}

bool HexInput::keyPressed(Uint8* const keyPress, const Uint32 tickCount) {
	if (!mCustomBinds.size()) { return false; }

	if (tickCount >= mTickLast + mTickSpan) {
		mKeysPrev &= ~mKeysLoop;
	}

		const auto pressKeys{ mKeysCurr & ~mKeysPrev };
	if (pressKeys) {
		const auto pressDiff{ pressKeys & ~mKeysLoop };
		const auto validKeys{ pressDiff ? pressDiff : mKeysLoop };

		mKeysLock |= validKeys;
		mTickLast  = tickCount;
		mTickSpan  = validKeys != mKeysLoop ? 20 : 5;
		mKeysLoop  = validKeys & ~(validKeys - 1);
		*keyPress  = static_cast<Uint8>(std::countr_zero(mKeysLoop));
	}
	return pressKeys;
}

bool HexInput::keyPressed(const std::size_t index, const std::size_t offset) const {
	return mKeysCurr & ~mKeysLock & 1 << ((index & 0xF) + offset);
}
