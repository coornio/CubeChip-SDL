/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <fstream>
#include <execution>

#include "HomeDirManager.hpp"

#include "../Assistants/SHA1.hpp"
#include "../Assistants/PathGetters.hpp"
#include "../Assistants/BasicLogger.hpp"

#include <SDL3/SDL_messagebox.h>

static auto getFileModTime(const fsPath& filePath) noexcept {
	std::error_code error;
	return std::filesystem::last_write_time(filePath, error);
}

/*==================================================================*/
	#pragma region HomeDirManager Class
/*==================================================================*/

HomeDirManager::HomeDirManager(const char* const org, const char* const app) noexcept {
	if (!getHomePath(org, app)) {
		showErrorBox("Filesystem Error", "Unable to get home directory!");
	} else {
		blog.initLogFile("program.log", getHomePath());
	}
}

void HomeDirManager::showErrorBox(const char* const title, const char* const message) noexcept {
	setErrorState(true);
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR, title,
		message, nullptr
	);
}

fsPath* HomeDirManager::addSystemDir(const fsPath& sub, const fsPath& sys) noexcept {
	if (sub.empty()) { return nullptr; }
	
	const fsPath newDir{ getHomePath() / sys / sub };

	auto it = std::find_if(
		std::execution::unseq,
		mDirectories.begin(), mDirectories.end(),
		[&newDir](const fsPath& dirEntry) {
			return dirEntry == newDir;
		}
	);

	if (it != mDirectories.end()) { return &(*it); }

	std::error_code error;

	std::filesystem::create_directories(newDir, error);
	if (!std::filesystem::exists(newDir, error) || error) {
		showErrorBox("Filesystem Error", (newDir.string() + "\nUnable to create subdirectories!").c_str());
		return nullptr;
	}

	mDirectories.push_back(newDir);
	return &mDirectories.back();
}

void HomeDirManager::clearCachedFileData() noexcept {
	mFilePath.clear();
	mFileSHA1.clear();
	mFileData.resize(0);
}

bool HomeDirManager::validateGameFile(const fsPath gamePath) noexcept {
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

	const auto tempTime{ getFileModTime(gamePath) };

	std::ifstream ifs(gamePath, std::ios::binary);
	mFileData.assign(std::istreambuf_iterator(ifs), {});

	if (tempTime != getFileModTime(gamePath)) {
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
