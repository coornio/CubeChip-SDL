/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../Libraries/nlohmann/json.hpp"
#include "../Assistants/BasicLogger.hpp"
#include "../Assistants/PathGetters.hpp"
#include "../Assistants/SimpleFileIO.hpp"
#include "CoreRegistry.hpp"

/*==================================================================*/

JsonDatabase CoreRegistry::sProgramDB{
	getBasePath()
};

/*==================================================================*/

bool CoreRegistry::validateProgramByHash(const char* fileData, ust fileSize, const Str& fileSHA1) noexcept {
	/* placeholder logic */
	[[maybe_unused]] const auto& _1{ fileData };
	[[maybe_unused]] const auto& _2{ fileSize };
	[[maybe_unused]] const auto& _3{ fileSHA1 };
	return false;
}

bool CoreRegistry::validateProgramByType(const char* fileData, ust fileSize, const Str& fileType) noexcept {
	const auto matchingCores{ findEligibleCores(fileType) };

	if (!matchingCores || matchingCores->empty()) {
		blog.newEntry(BLOG::WARN, "Unable to match Program to an existing System variant!");
		return false;
	} else {
		sEligible.clear();
		for (const auto& core : *matchingCores) {
			if (core.testProgram(fileData, fileSize)) {
				sEligible.push_back(core);
			}
		}

		if (sEligible.empty()) {
			blog.newEntry(BLOG::WARN, "Program rejected by eligible System variants!");
			return false;
		} else {
			return true;
		}
	}
}

bool CoreRegistry::validateProgram(const char* fileData, ust fileSize, const Str& fileType, const Str& fileSHA1) noexcept {
	if (fileSHA1.empty()) {
		return validateProgramByType(fileData, fileSize, fileType);
	} else {
		return validateProgramByType(fileData, fileSize, fileType); // placeholder
		//return validateProgramByHash(fileData, fileSize, fileSHA1);
	}
}

void CoreRegistry::registerCore(CoreConstructor&& ctor, ProgramTester&& tester, FileExtList exts) noexcept {
	CoreDetails reg{ ctor, tester, std::move(exts), };
	for (const auto& ext : reg.fileExtensions) {
		try { sRegistry[ext].push_back(reg); }
		catch (const std::exception& e) {
			blog.newEntry(BLOG::ERROR, "Exception triggered trying to register Emulator Core! \n{}", e.what());
		}
	}
}

const CoreRegList* CoreRegistry::findEligibleCores(const Str& ext) noexcept {
	auto it = sRegistry.find(ext);
	return it != sRegistry.end() ? &it->second : nullptr;
}

EmuInterface* CoreRegistry::constructCore(size_type idx) noexcept {
	try {
		// this will later need to handle choosing a specific core out of all available
		// rather than the first one present, adding flexibility
		sCurrentCore = sEligible[idx];
		return sCurrentCore.constructCore();
	} catch (const std::exception& e) {
		blog.newEntry(BLOG::ERROR, "Exception triggered trying to construct Emulator Core! \n{}", e.what());
		return nullptr;
	}
}

void CoreRegistry::loadProgramDB(const Path& dbPath) noexcept {
	static const auto defaultPath{ Path(getBasePath()) / "programDB.json" };

	if (auto dbData = ::readFileData(dbPath)) {
		try {
			sProgramDB = JsonDatabase::parse(dbData->begin(), dbData->end());
		} catch (const JsonDatabase::parse_error& e) {
			blog.newEntry(BLOG::ERROR, "Exception triggered trying to parse default JSON DB! \n{}", e.what());
		}
		return;
	}

	if (auto dbData = ::readFileData(defaultPath)) {
		try {
			sProgramDB = JsonDatabase::parse(dbData->begin(), dbData->end());
		} catch (const JsonDatabase::parse_error& e) {
			blog.newEntry(BLOG::ERROR, "Exception triggered trying to parse custom JSON DB! \n{}", e.what());
		}
		return;
	}
}
