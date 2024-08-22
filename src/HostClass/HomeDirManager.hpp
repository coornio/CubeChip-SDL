/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstdint>
#include <string>
#include <filesystem>
#include <string_view>

#include "../Assistants/BasicHome.hpp"

class HomeDirManager final : public BasicHome {

	using GameValidator = bool (*)(std::uint64_t, std::string_view, std::string_view);
	using FileModTime   = std::filesystem::file_time_type;
	using FilePath      = std::filesystem::path;
	using FileSize      = std::uintmax_t;

	//FilePath    mFullPath{};
	std::string mFilePath{};
	std::string mFileName{};
	std::string mFileStem{};
	std::string mFileExts{};
	std::string mFileSHA1{};
	FileModTime mFileTime{};
	FileSize    mFileSize{};

	GameValidator checkGame{};

public:
	FilePath    permRegs{};
	void addDirectory();

	auto getFilePath() const noexcept { return mFilePath; }
	auto getFileName() const noexcept { return mFileName; }
	auto getFileStem() const noexcept { return mFileStem; }
	auto getFileExts() const noexcept { return mFileExts; }
	auto getFileSHA1() const noexcept { return mFileSHA1; }
	auto getFileTime() const noexcept { return mFileTime; }
	auto getFileSize() const noexcept { return mFileSize; }

	HomeDirManager(const std::string_view);

	void setValidator(GameValidator func) noexcept { checkGame = func; }

	void clearCachedFileData() noexcept;
	bool validateGameFile(const char*) noexcept;
};
