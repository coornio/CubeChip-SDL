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
	const auto matchingCores{ findCore(fileType) };

	if (!matchingCores || matchingCores->empty()) {
		blog.newEntry(BLOG::WARN, "Cannot match Game to a supported system/platform!");
		return false;
	} else {
		mEligible.clear();
		for (const auto& core : *matchingCores) {
			if (core.testProgram(fileData, fileSize)) {
				mEligible.push_back(core);
			}
		}
		return !mEligible.empty();
	}
}

EmuInterface* CoreRegistry::constructCore() noexcept {
	try {
		// this will later need to handle choosing a specific core out of all available
		// rather than the first one present, adding flexibility
		mCurrentCore = mEligible.front();
		return mCurrentCore.constructCore();
	} catch (const std::exception&) {
		blog.newEntry(BLOG::ERROR, "Failed to construct Game Core!");
		return nullptr;
	}
}
