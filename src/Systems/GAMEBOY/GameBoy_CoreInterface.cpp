/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../../Assistants/FrameLimiter.hpp"
#include "../../Assistants/BasicInput.hpp"
#include "../../Assistants/HomeDirManager.hpp"
#include "../../Assistants/BasicAudioSpec.hpp"

#include "GameBoy_CoreInterface.hpp"

/*==================================================================*/

GameBoy_CoreInterface::GameBoy_CoreInterface() noexcept
	: ASB{ std::make_unique<AudioSpecBlock>(SDL_AUDIO_S8, 1, 48'000, 4) }
{
	if ((sSavestatePath = HDM->addSystemDir("savestate", "GAMEBOY"))) {
		*sSavestatePath /= HDM->getFileSHA1();
	}

	ASB->resumeStreams();
	loadPresetBinds();
}

GameBoy_CoreInterface::~GameBoy_CoreInterface() noexcept {}

/*==================================================================*/

void GameBoy_CoreInterface::processFrame() {
	if (Pacer->checkTime()) {
		if (isSystemStopped()) { return; }

		updateKeyStates();
		instructionLoop();
		renderAudioData();
		renderVideoData();
		writeStatistics();
	}
}

void GameBoy_CoreInterface::loadPresetBinds() {
	static constexpr auto _{ SDL_SCANCODE_UNKNOWN };
	static constexpr SimpleKeyMapping defaultKeyMappings[]{
		{0x7, KEY(G), _}, // START
		{0x6, KEY(F), _}, // SELECT
		{0x5, KEY(Q), _}, // B
		{0x4, KEY(E), _}, // A
		{0x3, KEY(S), _}, // ↓
		{0x2, KEY(W), _}, // ↑
		{0x1, KEY(A), _}, // ←
		{0x0, KEY(D), _}, // →
	};

	loadCustomBinds(std::span(defaultKeyMappings));
}

u32  GameBoy_CoreInterface::getKeyStates() const {
	auto keyStates{ 0u };

	Input->updateStates();

	for (const auto& mapping : mCustomBinds) {
		if (Input->areAnyHeld(mapping.key, mapping.alt)) {
			keyStates |= 1u << mapping.idx;
		}
	}

	return keyStates;
}

void GameBoy_CoreInterface::copyGameToMemory(u8* dest) noexcept {
	std::copy_n(
		std::execution::unseq,
		HDM->getFileData(),
		HDM->getFileSize(),
		dest
	);
}
