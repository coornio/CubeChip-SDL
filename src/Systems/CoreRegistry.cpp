/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../Libraries/nlohmann/json.hpp"
#include "../Assistants/BasicLogger.hpp"
#include "CoreRegistry.hpp"

using json = nlohmann::json;

/*==================================================================*/

//nlohmann::json GameFileChecker::sEmuConfig{};

/*==================================================================*/

bool CoreRegistry::validateProgram(const char* fileData, ust fileSize, const Str& fileType) noexcept {
	const auto matchingCores{ findEligibleCores(fileType) };

	if (!matchingCores || matchingCores->empty()) {
		blog.newEntry(BLOG::WARN, "Unable to match Program to an existing System variant!");
		return false;
	} else {
		mEligible.clear();
		for (const auto& core : *matchingCores) {
			if (core.testProgram(fileData, fileSize)) {
				mEligible.push_back(core);
			}
		}

		if (mEligible.empty()) {
			blog.newEntry(BLOG::WARN, "Program rejected by eligible System variants!");
			return false;
		} else {
			return true;
		}
	}
}

EmuInterface* CoreRegistry::constructCore(size_type idx) noexcept {
	try {
		// this will later need to handle choosing a specific core out of all available
		// rather than the first one present, adding flexibility
		mCurrentCore = mEligible[idx];
		return mCurrentCore.constructCore();
	} catch (const std::exception& e) {
		blog.newEntry(BLOG::ERROR, "Exception triggered trying to construct Emulator Core! \n{}", e.what());
		return nullptr;
	}
}
