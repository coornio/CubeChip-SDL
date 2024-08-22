/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <fstream>

#include "HomeDirManager.hpp"
#include "../Assistants/BasicLogger.hpp"
#include "../Assistants/PathExceptionClass.hpp"
#include "../Assistants/SHA1.hpp"

using namespace blogger;

HomeDirManager::HomeDirManager(const std::string_view homeDirName) try
	: BasicHome{ homeDirName }
{
	try {
		blog.setStdLogFile("program.log", getHome());
		addDirectory();
	} catch (const std::exception& e) {
		BasicHome::showErrorBox(e.what(), "Fatal Initialization Error");
		throw;
	}
} catch (const std::exception& e) {
	BasicHome::showErrorBox(e.what(), "Fatal Initilization Error");
	throw;
}

void HomeDirManager::clearCachedFileData() noexcept {
	path.clear(); file.clear();
	name.clear(); type.clear();
	sha1.clear(); size = 0;
}

void HomeDirManager::addDirectory() {
	permRegs = getHome() / "permRegs";
	std::filesystem::create_directories(permRegs);
	if (!std::filesystem::exists(permRegs)) {
		throw PathException("Could not create subdir: ", permRegs);
	}
}

bool HomeDirManager::validateGameFile(const char* filepath) noexcept {
	if (!filepath) { return false; }
	namespace fs = std::filesystem;
	std::error_code error;

	const fs::path fspath{ filepath };
	blog.newEntry(BLOG::INFO, "Attempting to access file: " + fspath.string());

	if (!fs::exists(fspath, error) || error) {
		blog.newEntry(BLOG::WARN, "Unable to locate path!" + error.message());
		return false;
	}

	if (!fs::is_regular_file(fspath, error) || error) {
		blog.newEntry(BLOG::WARN, "Provided path is not to a file!");
		return false;
	}
	
	const std::uint64_t fileSize{ fs::file_size(fspath, error) };
	if (error) {
		blog.newEntry(BLOG::ERROR, "Unable to read file!");
		return false;
	}
	if (fileSize == 0) {
		blog.newEntry(BLOG::WARN, "File must not be empty!");
		return false;
	}

	const auto tempPath{ fspath.string() };
	const auto tempType{ fspath.extension().string() };
	const auto tempSHA1{ SHA1::from_file(tempPath) };

	const bool gameApproved{ checkGame(fileSize, tempType, tempSHA1) };

	if (gameApproved) {
		path = tempPath;
		file = fspath.filename().string();
		name = fspath.stem().string();
		type = tempType;
		sha1 = tempSHA1;
		size = fileSize;

		blog.newEntry(BLOG::INFO, "File is a valid game!");
	} else {
		blog.newEntry(BLOG::INFO, "File is not a valid game!");
	}

	return gameApproved;
}
