/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

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
	
	const Path newDirPath{ getHomePath() / sys / sub };

	auto it = std::find_if(
		std::begin(mDirectories), std::end(mDirectories),
		[&newDirPath](const Path& dirEntry) {
			return dirEntry == newDirPath;
		}
	);

	if (it != mDirectories.end()) { return &(*it); }

	std::error_code error;

	std::filesystem::create_directories(newDirPath, error);
	if (!std::filesystem::exists(newDirPath, error) || error) {
		mSuccessful = false;
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR, "Filesystem Error",
			(newDirPath.string() + "\nUnable to create subdirectories!").c_str(), nullptr
		);
		return nullptr;
	}

	mDirectories.push_back(newDirPath);
	return &mDirectories.back();
}

void HomeDirManager::clearCachedFileData() noexcept {
	mFilePath.clear();
	mFileSHA1.clear();
	mFileData.resize(0);
}

bool HomeDirManager::validateGameFile(const Path gamePath) noexcept {

	const auto fileExists{ fs::is_regular_file(gamePath) };
	if (!fileExists || !fileExists.value()) {
		blog.newEntry(BLOG::WARN, "Path is ineligible: \"{}\" [{}]",
			gamePath.string(), fileExists.error().message());
		return false;
	}

	const auto fileSize{ fs::file_size(gamePath) };
	if (!fileSize) {
		blog.newEntry(BLOG::WARN, "Path is ineligible: \"{}\" [{}]",
			gamePath.string(), fileExists.error().message());
		return false;
	}
	if (fileSize.value() == 0) {
		blog.newEntry(BLOG::WARN, "Game file must not be empty!");
		return false;
	}
	if (fileSize.value() >= ::CalcBytes(32, MiB)) {
		blog.newEntry(BLOG::WARN, "Game file is too large!");
		return false;
	}

	auto fileData{ ::readFileData(gamePath) };
	if (!fileData) {
		blog.newEntry(BLOG::WARN, "Path is ineligible: \"{}\" [{}]",
			gamePath.string(), fileExists.error().message());
		return false;
	}

	mFileData = std::move(fileData.value());

	const auto tempSHA1{ SHA1::from_data(mFileData) };
	blog.newEntry(BLOG::INFO, "SHA1: {}", tempSHA1);

	if (checkGame(
		std::data(mFileData), std::size(mFileData),
		gamePath.extension().string(), tempSHA1
	)) {
		mFilePath = gamePath;
		mFileSHA1 = tempSHA1;
		return true;
	} else {
		return false;
	}
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
