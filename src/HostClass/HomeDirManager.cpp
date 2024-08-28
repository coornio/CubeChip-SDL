/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <fstream>

#include "HomeDirManager.hpp"

#include "../Assistants/BasicLogger.hpp"
#include "../Assistants/SHA1.hpp"

#include <SDL3/SDL_messagebox.h>

using namespace blogger;

/*==================================================================*/
	#pragma region HomeDirManager Class
/*==================================================================*/

HomeDirManager::HomeDirManager(const char* const org, const char* const app) noexcept {
	if (!HDM::getHomePath(org, app)) {
		errorTriggered = true;
		showErrorBox("Filesystem Error", "Unable to get home directory!");
	} else {
		blog.initLogFile("program.log", HDM::getHomePath());
		addDirectory(); // XXX needs fixing
	}
}

bool HomeDirManager::showErrorBox(const char* const title, const char* const message) noexcept {
	return SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, nullptr);
}

void HomeDirManager::clearCachedFileData() noexcept {
	mFilePath.clear();
	mFileSHA1.clear();
	mFileData.resize(0);
}

void HomeDirManager::addDirectory() { // XXX needs fixing
	if (getSelfStatus()) { return; }

	permRegs.assign(HDM::getHomePath()) /= "permRegs";

	std::filesystem::create_directories(permRegs);
	if (!std::filesystem::exists(permRegs)) {
		errorTriggered = true;
		showErrorBox("Filesystem Error", "Unable to create subdirectory!");
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

	const auto tempTime{ HDM::getFileTime(gamePath) };

	std::ifstream ifs(gamePath, std::ios::binary);
	mFileData.assign(std::istreambuf_iterator(ifs), {});

	if (tempTime != HDM::getFileTime(gamePath)) {
		blog.newEntry(BLOG::WARN, "File was modified while reading!");
		return false;
	}

	if (!getFileSize()) {
		blog.newEntry(BLOG::WARN, "File must not be empty!");
		return false;
	}

	const auto tempSHA1{ SHA1::from_span(mFileData) };
	const bool gameApproved{ checkGame(getFileSize(), gamePath.extension().string(), tempSHA1)};

	if (gameApproved) {
		mFilePath = gamePath;
		mFileSHA1 = tempSHA1;
	}

	if (gameApproved) {
		blog.newEntry(BLOG::INFO, "File is a valid game!");
	} else {
		blog.newEntry(BLOG::INFO, "File is not a valid game!");
	}

	return gameApproved;
}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
