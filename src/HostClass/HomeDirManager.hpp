/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <filesystem>

/*==================================================================*/
	#pragma region HomeDirManager Singleton Class
/*==================================================================*/

class HomeDirManager final {
	HomeDirManager(const char* const org, const char* const app) noexcept;
	~HomeDirManager() noexcept = default;
	HomeDirManager(const HomeDirManager&) = delete;
	HomeDirManager& operator=(const HomeDirManager&) = delete;

	using GameValidator = bool (*)(const std::size_t, const std::string&, const std::string&);

	std::filesystem::path mFilePath{};
	std::string           mFileSHA1{};

	std::vector<char>
		mFileData{};

	std::vector<std::filesystem::path>
		mDirectories{};

	GameValidator checkGame{};
	bool errorEncountered{};

public:
	static auto& create(const char* const org, const char* const app) noexcept {
		static HomeDirManager self(org, app);
		return self;
	}

	void setErrorState(const bool state) noexcept { errorEncountered = state; }
	bool getErrorState()           const noexcept { return errorEncountered;  }

	static bool showErrorBox(const char* const, const char* const) noexcept;

	auto addSystemDir(
		const std::filesystem::path& sub,
		const std::filesystem::path& sys = {}
	) noexcept -> std::filesystem::path*;

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
	bool validateGameFile(const std::filesystem::path) noexcept;
};

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

extern HomeDirManager& HDM;
