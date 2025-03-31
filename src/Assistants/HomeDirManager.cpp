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

HomeDirManager::HomeDirManager(const char* homePath) noexcept {
	if (homePath) { blog.initLogFile("program.log", homePath); }
}

HomeDirManager* HomeDirManager::create(const char* const org, const char* const app) noexcept {
	static HomeDirManager self(getHomePath(org, app));
	return getHomePath() ? &self : nullptr;
}

Path* HomeDirManager::addSystemDir(const Path& sub, const Path& sys) noexcept {
	if (sub.empty()) { return nullptr; }
	
	const auto newDirPath{ ::getHomePath() / sys / sub };

	const auto it{ std::find_if(EXEC_POLICY(unseq)
		mDirectories.begin(), mDirectories.end(),
		[&newDirPath](const Path& dirEntry) {
			return dirEntry == newDirPath;
		}
	) };

	if (it != mDirectories.end()) { return &(*it); }

	if (const auto dirCreated{ fs::create_directories(newDirPath) }) {
		mDirectories.push_back(newDirPath);
		return &mDirectories.back();
	} else {
		blog.newEntry(BLOG::ERROR, "Unable to create directory: \"{}\" [{}]",
			newDirPath.string(), dirCreated.error().message());
		return nullptr;
	}
}

void HomeDirManager::clearCachedFileData() noexcept {
	mFilePath.clear();
	mFileSHA1.clear();
	mFileData.resize(0);
}

bool HomeDirManager::validateGameFile(const Path& gamePath) noexcept {
	const auto fileExists{ fs::is_regular_file(gamePath) };
	if (!fileExists) {
		blog.newEntry(BLOG::WARN, "Path is ineligible: \"{}\" [{}]",
			gamePath.string(), fileExists.error().message());
		return false;
	}
	if (!fileExists.value()) {
		blog.newEntry(BLOG::WARN, "{}: \"{}\"",
			"Path is not a regular file", gamePath.string());
		return false;
	}

	const auto fileSize{ fs::file_size(gamePath) };
	if (!fileSize) {
		blog.newEntry(BLOG::WARN, "Path is ineligible: \"{}\" [{}]",
			gamePath.string(), fileExists.error().message());
		return false;
	}
	if (fileSize.value() == 0) {
		blog.newEntry(BLOG::WARN, "File must not be empty!");
		return false;
	}
	if (fileSize.value() >= ::CalcBytes(32, MiB)) {
		blog.newEntry(BLOG::WARN, "File is too large!");
		return false;
	}

	auto fileData{ ::readFileData(gamePath) };
	if (!fileData) {
		blog.newEntry(BLOG::WARN, "Path is ineligible: \"{}\" [{}]",
			gamePath.string(), fileData.error().message());
		return false;
	} else {
		mFileData = std::move(fileData.value());
	}

	const auto tempSHA1{ SHA1::from_data(mFileData) };
	blog.newEntry(BLOG::INFO, "File SHA1: {}", tempSHA1);

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
