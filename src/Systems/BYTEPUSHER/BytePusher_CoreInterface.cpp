/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../Assistants/BasicInput.hpp"
#include "../../Assistants/HomeDirManager.hpp"
#include "../../Assistants/BasicAudioSpec.hpp"

#include "BytePusher_CoreInterface.hpp"

/*==================================================================*/

fsPath* BytePusher_CoreInterface::sSavestatePath{};

BytePusher_CoreInterface::BytePusher_CoreInterface() noexcept
	: ASB{ std::make_unique<AudioSpecBlock>(SDL_AUDIO_S8, 1, 15'360) }
{
	sSavestatePath = HDM->addSystemDir("savestate", "BYTEPUSHER");
	if (!sSavestatePath) { addCoreState(EmuState::FAILED); }

	loadPresetBinds();
}

BytePusher_CoreInterface::~BytePusher_CoreInterface() noexcept {}

/*==================================================================*/

void BytePusher_CoreInterface::processFrame() {
	if (isSystemStopped()) { return; }
	else [[likely]] { ++mTotalFrames; }

	instructionLoop();
	renderAudioData();
	renderVideoData();
}

void BytePusher_CoreInterface::loadPresetBinds() {
	static constexpr auto _{ SDL_SCANCODE_UNKNOWN };
	static constexpr SimpleKeyMapping defaultKeyMappings[]{
		{0x1, KEY(1), _}, {0x2, KEY(2), _}, {0x3, KEY(3), _}, {0xC, KEY(4), _},
		{0x4, KEY(Q), _}, {0x5, KEY(W), _}, {0x6, KEY(E), _}, {0xD, KEY(R), _},
		{0x7, KEY(A), _}, {0x8, KEY(S), _}, {0x9, KEY(D), _}, {0xE, KEY(F), _},
		{0xA, KEY(Z), _}, {0x0, KEY(X), _}, {0xB, KEY(C), _}, {0xF, KEY(V), _},
	};

	loadCustomBinds(defaultKeyMappings);
}

void BytePusher_CoreInterface::loadCustomBinds(std::span<const SimpleKeyMapping> binds) {
	mCustomBinds.assign(binds.begin(), binds.end());
}

u32  BytePusher_CoreInterface::getKeyStates() const {
	auto keyStates{ 0u };

	for (const auto& mapping : mCustomBinds) {
		if (binput::kb.areAnyHeld(mapping.key, mapping.alt)) {
			keyStates |= 1u << mapping.idx;
		}
	}

	return keyStates;
}

void BytePusher_CoreInterface::copyGameToMemory(u8* dest) noexcept {
	std::copy_n(
		std::execution::unseq,
		HDM->getFileData(),
		HDM->getFileSize(),
		dest
	);
}
