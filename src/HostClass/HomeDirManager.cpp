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
	//mFilePath.clear();
	//mFileName.clear();
	//mFileStem.clear();
	//mFileExts.clear();
	mFilePath.clear();
	mFileSHA1.clear();
	mFileTime = {};
	mFileSize = {};
}

void HomeDirManager::addDirectory() {
	permRegs = getHome() / "permRegs";
	std::filesystem::create_directories(permRegs);
	if (!std::filesystem::exists(permRegs)) {
		throw PathException("Could not create subdir: ", permRegs);
	}
}

bool HomeDirManager::validateGameFile(const FilePath gamePath) noexcept {
	if (gamePath.empty()) { return false; }
	namespace fs = std::filesystem;
	std::error_code error;

	blog.newEntry(BLOG::INFO, "Attempting to access file: " + gamePath.string());

	if (!fs::exists(gamePath, error) || error) {
		blog.newEntry(BLOG::WARN, "Unable to locate path!" + error.message());
		return false;
	}

	if (!fs::is_regular_file(gamePath, error) || error) {
		blog.newEntry(BLOG::WARN, "Provided path is not to a file!");
		return false;
	}
	
	const auto tempSize{ fs::file_size(gamePath, error) };
	if (error) {
		blog.newEntry(BLOG::ERROR, "Unable to read file!");
		return false;
	}
	if (tempSize == 0) {
		blog.newEntry(BLOG::WARN, "File must not be empty!");
		return false;
	}

	const auto tempTime{ HDM::getFileTime(gamePath) };
	const auto tempSHA1{ SHA1::from_file(gamePath.string()) };

	if (tempTime != HDM::getFileTime(gamePath)) {
		blog.newEntry(BLOG::WARN, "File was modified while reading!");
		return false;
	}

	const bool gameApproved{ checkGame(tempSize, gamePath.extension().string(), tempSHA1) };

	if (gameApproved) {
		mFilePath = gamePath;
		mFileSHA1 = tempSHA1;
		mFileTime = tempTime;
		mFileSize = tempSize;
	}

	if (gameApproved) {
		blog.newEntry(BLOG::INFO, "File is a valid game!");
	} else {
		blog.newEntry(BLOG::INFO, "File is not a valid game!");
	}

	return gameApproved;
}
