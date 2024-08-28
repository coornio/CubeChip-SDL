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

#include <SDL3/SDL_filesystem.h>

/*==================================================================*/
	#pragma region HomeDirManager Class
/*==================================================================*/

class HomeDirManager final {
	using GameValidator = bool (*)(const std::size_t, const std::string&, const std::string&);
	using FilePath      = std::filesystem::path;

	FilePath    mFilePath{};
	std::string mFileSHA1{};

	GameValidator checkGame{};

	std::vector<char> mFileData{};

	bool errorTriggered{};

public:
	HomeDirManager(const char* const org, const char* const app) noexcept;

	bool getSelfStatus() const noexcept { return errorTriggered; }

	static bool showErrorBox(const char* const, const char* const) noexcept;

	FilePath permRegs{}; // XXX needs fixing
	void addDirectory(); // XXX needs fixing

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
	bool validateGameFile(const FilePath) noexcept;
};

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

namespace HDM {
	inline auto getHomePath(
		const char* const org = nullptr,
		const char* const app = nullptr
	) {
		static const char* const homePath{ SDL_GetPrefPath(org, app) };
		return homePath;
	}

	inline auto getFileTime(const std::filesystem::path& filePath) noexcept {
		std::error_code error;
		return std::filesystem::last_write_time(filePath, error);
	}
}
