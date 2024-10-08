/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../Assistants/Well512.hpp"

#include "EmuInterface.hpp"

/*==================================================================*/

u32 EmuInterface::mGlobalState{ EmuState::NORMAL };

HomeDirManager* EmuInterface::HDM{};
BasicVideoSpec* EmuInterface::BVS{};
BasicAudioSpec* EmuInterface::BAS{};

Well512* EmuInterface::Wrand{};

EmuInterface::EmuInterface() noexcept {
	static Well512 sWrand;
	Wrand = &sWrand;
}

EmuInterface::~EmuInterface() noexcept {
	subSystemState(EmuState::PAUSED);
}
