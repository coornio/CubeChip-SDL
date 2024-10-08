/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include <vector>

#include "Typedefs.hpp"

/*==================================================================*/
	#pragma region HomeDirManager Singleton Class

class HomeDirManager final {
	HomeDirManager(const char* const org, const char* const app) noexcept;
	~HomeDirManager() noexcept = default;
	HomeDirManager(const HomeDirManager&) = delete;
	HomeDirManager& operator=(const HomeDirManager&) = delete;

	using GameValidator = bool (*)(const std::size_t, const std::string&, const std::string&);

	fsPath      mFilePath{};
	std::string mFileSHA1{};

	std::vector<char>
		mFileData{};

	std::vector<fsPath>
		mDirectories{};

	GameValidator checkGame{};

	static bool& errorState() noexcept {
		static bool errorEncountered{};
		return errorEncountered;
	}

public:
	static auto* create(const char* const org, const char* const app) noexcept {
		static HomeDirManager self(org, app);
		return errorState() ? nullptr : &self;
	}

	static void setErrorState(const bool state) noexcept { errorState() = state; }
	static bool getErrorState()                 noexcept { return errorState();  }

	static void showErrorBox(const char* const, const char* const) noexcept;

	fsPath* addSystemDir(
		const fsPath& sub,
		const fsPath& sys = {}
	) noexcept;

	auto getFullPath() const noexcept { return mFilePath; }
	auto getFilePath() const noexcept { return mFilePath.string(); }
	auto getFileName() const noexcept { return mFilePath.filename().string(); }
	auto getFileStem() const noexcept { return mFilePath.stem().string(); }
	auto getFileExts() const noexcept { return mFilePath.extension().string(); }
	auto getFileSize() const noexcept { return mFileData.size(); }
	auto getFileData() const noexcept { return mFileData.data(); }
	auto getFileSHA1() const noexcept { return mFileSHA1; }

	void setValidator(GameValidator func) noexcept { checkGame = func; }

	void clearCachedFileData() noexcept;
	bool validateGameFile(const fsPath) noexcept;
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
