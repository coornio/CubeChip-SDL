/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <fstream>
#include <execution>

#include "HomeDirManager.hpp"

#include "../Assistants/SHA1.hpp"
#include "../Assistants/SimpleFileIO.hpp"
#include "../Assistants/PathGetters.hpp"
#include "../Assistants/BasicLogger.hpp"

#include <SDL3/SDL_messagebox.h>

/*==================================================================*/
	#pragma region HomeDirManager Class

HomeDirManager::HomeDirManager(const char* const org, const char* const app) noexcept {
	mSuccessful = getHomePath(org, app);
	if (!mSuccessful) {
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR, "Filesystem Error",
			"Unable to get home directory!", nullptr
		);
	} else {
		blog.initLogFile("program.log", getHomePath());
	}
}

Path* HomeDirManager::addSystemDir(const Path& sub, const Path& sys) noexcept {
	if (sub.empty()) { return nullptr; }
	
	const Path newDir{ getHomePath() / sys / sub };

	auto it = std::find_if(
		std::execution::unseq,
		mDirectories.begin(), mDirectories.end(),
		[&newDir](const Path& dirEntry) {
			return dirEntry == newDir;
		}
	);

	if (it != mDirectories.end()) { return &(*it); }

	std::error_code error;

	std::filesystem::create_directories(newDir, error);
	if (!std::filesystem::exists(newDir, error) || error) {
		mSuccessful = false;
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR, "Filesystem Error",
			(newDir.string() + "\nUnable to create subdirectories!").c_str(), nullptr
		);
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

bool HomeDirManager::validateGameFile(const Path gamePath) noexcept {
	std::error_code error;

	if (!::doesFileExist(gamePath, error) || error) {
		blog.newEntry(BLOG::WARN, "Path is ineligible: {}", error.message());
		return false;
	}

	const auto fileSize{ ::getFileSize(gamePath, error) };
	if (error) {
		blog.newEntry(BLOG::WARN, "Path is ineligible: {}", error.message());
		return false;
	}

	if (fileSize == 0) {
		blog.newEntry(BLOG::WARN, "Game file must not be empty!");
		return false;
	}

	if (fileSize >= 33'554'432) { // 32 MB upper limit
		blog.newEntry(BLOG::WARN, "Game file is too large!");
		return false;
	}

	mFileData = std::move(::readFileData(gamePath, error));
	if (error) {
		blog.newEntry(BLOG::WARN, "Path is ineligible: {}", error.message());
		return false;
	}

	const auto tempSHA1{ SHA1::from_span(mFileData) };

	if (checkGame(mFileData, gamePath.extension().string(), tempSHA1)) {
		mFilePath = gamePath;
		mFileSHA1 = tempSHA1;
		return true;
	} else {
		return false;
	}
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
