/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <string_view>

#include "../Assistants/BasicHome.hpp"

class HomeDirManager final : public BasicHome {
	using GameValidator = bool (*)(std::uint64_t, std::string_view, std::string_view);
	using FileModTime   = std::filesystem::file_time_type;
	using FilePath      = std::filesystem::path;

	FilePath    mFilePath{};
	std::string mFileSHA1{};
	FileModTime mFileTime{};

	GameValidator checkGame{};

	std::vector<char> mFileData{};

public:
	HomeDirManager(const std::string_view);

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
	auto getFileTime() const noexcept { return mFileTime; }

	void setValidator(GameValidator func) noexcept { checkGame = func; }

	void clearCachedFileData() noexcept;
	bool validateGameFile(const FilePath) noexcept;
};

namespace HDM {
	inline auto getFileTime(const std::filesystem::path& filePath) noexcept {
		std::error_code error;
		return std::filesystem::last_write_time(filePath, error);
	}
}
