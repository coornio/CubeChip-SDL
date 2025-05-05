/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../Assistants/FrameLimiter.hpp"
#include "../../Assistants/BasicInput.hpp"
#include "../../Assistants/HomeDirManager.hpp"
#include "../../Assistants/BasicAudioSpec.hpp"

#include "BytePusher_CoreInterface.hpp"

/*==================================================================*/

BytePusher_CoreInterface::BytePusher_CoreInterface() noexcept
	: mAudio{ AUDIOFORMAT::S16, 1, 15'360, STREAM::COUNT }
{
	if ((sSavestatePath = HDM->addSystemDir("savestate", "BYTEPUSHER"))) {
		*sSavestatePath /= HDM->getFileSHA1();
	}

	mAudio.resumeStreams();
	loadPresetBinds();
}

/*==================================================================*/

void BytePusher_CoreInterface::mainSystemLoop() {
	if (Pacer->checkTime()) {
		if (isSystemRunning())
			[[unlikely]] { return; }

		instructionLoop();
		renderAudioData();
		renderVideoData();
		writeStatistics();
	}
}

void BytePusher_CoreInterface::loadPresetBinds() {
	static constexpr auto _{ SDL_SCANCODE_UNKNOWN };
	static constexpr SimpleKeyMapping defaultKeyMappings[]{
		{0x1, KEY(1), _}, {0x2, KEY(2), _}, {0x3, KEY(3), _}, {0xC, KEY(4), _},
		{0x4, KEY(Q), _}, {0x5, KEY(W), _}, {0x6, KEY(E), _}, {0xD, KEY(R), _},
		{0x7, KEY(A), _}, {0x8, KEY(S), _}, {0x9, KEY(D), _}, {0xE, KEY(F), _},
		{0xA, KEY(Z), _}, {0x0, KEY(X), _}, {0xB, KEY(C), _}, {0xF, KEY(V), _},
	};

	loadCustomBinds(std::span(defaultKeyMappings));
}

u32  BytePusher_CoreInterface::getKeyStates() const {
	auto keyStates{ 0u };

	Input->updateStates();

	for (const auto& mapping : mCustomBinds) {
		if (Input->areAnyHeld(mapping.key, mapping.alt)) {
			keyStates |= 1u << mapping.idx;
		}
	}

	return keyStates;
}

void BytePusher_CoreInterface::copyGameToMemory(u8* dest) noexcept {
	std::copy_n(EXEC_POLICY(unseq)
		HDM->getFileData(),
		HDM->getFileSize(),
		dest
	);
}
