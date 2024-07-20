/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <bit>
#include <bitset>
#include <iostream>

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
	if (mCustomBinds.size()) {
		mKeysPrev = mKeysCurr;
		mKeysCurr = 0;

		for (const auto& mapping : mCustomBinds) {
			if (bic::kb.areAnyHeld(mapping.key, mapping.alt)) {
				mKeysCurr |= 1 << mapping.idx;
			}
		}
		// unlock key bits if they underwent state change
		mKeysLock &= ~(mKeysPrev ^  mKeysCurr);
		//mKeysLoop &= ~(mKeysPrev ^  mKeysCurr);
		//mKeysLock = mKeysLock & ~(mKeysPrev & ~mKeysCurr);

		const auto pressKeys{ mKeysCurr & ~mKeysPrev & ~mKeysLoop };
		const auto validKeys{ pressKeys ?  pressKeys :  mKeysLoop };

		//mWaitFrames = pressKeys ? 16 : 3;
		//mKeysLoop   = validKeys & ~(validKeys - 1);

		//std::cout << std::bitset<32>(mKeysLoop) << std::endl;
	}
}

bool HexInput::keyPressed(Uint8* returnKey, const Uint32 counter) {
	if (!mCustomBinds.size()) { return false; }

	if (mKeysLock & mKeysLoop) {
		if (counter > mFrameStamp + mWaitFrames + 1) {
			printf("\nframe counter passed the limit ");
			mKeysPrev &= ~mKeysLoop;
		} else {
			printf("|");
		}
	} else {
		printf(".");
		mKeysLoop = 0;
	}

	const auto pressKeys{ mKeysCurr & ~mKeysPrev };
	const auto validKeys{ pressKeys & ~mKeysLoop ?  pressKeys :  mKeysLoop };

	//const auto pressedKeysY{ mKeysCurr & ~mKeysPrev };
	//const auto lowOrderKeyY{ pressedKeysY & ~(pressedKeysY - 1) };
	if (pressKeys) {
		const bool differentKey{ validKeys != mKeysLoop };

		mKeysLock  |= validKeys;
		//mKeysLock |= mKeysLoop;
		mFrameStamp = counter;

		mWaitFrames = differentKey ? 16 : 3;
		mKeysLoop = validKeys;
		*returnKey = static_cast<Uint8>(std::countr_zero(mKeysLoop));

		return true;
	} else {
		return false;
	}
}

bool HexInput::keyPressed(const std::size_t index, const std::size_t offset) const {
	return mKeysCurr & ~mKeysLock & 1 << ((index & 0xF) + offset);
}
