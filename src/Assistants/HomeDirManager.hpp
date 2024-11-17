/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <span>
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

	using GameValidator = bool (*)(
		std::span<const char>,
		const Str&,
		const Str&
	) noexcept;

	Path mFilePath{};
	Str  mFileSHA1{};

	std::vector<char>
		mFileData{};

	std::vector<Path>
		mDirectories{};

	GameValidator checkGame{};

	static inline bool mSuccessful{ true };

public:
	static auto* create(const char* const org, const char* const app) noexcept {
		static HomeDirManager self(org, app);
		return mSuccessful ? &self : nullptr;
	}

	static bool isSuccessful() noexcept { return mSuccessful; }

	Path* addSystemDir(
		const Path& sub,
		const Path& sys = {}
	) noexcept;

	auto getFullPath() const noexcept { return mFilePath; }
	auto getFilePath() const noexcept { return mFilePath.string(); }
	auto getFileName() const noexcept { return mFilePath.filename().string(); }
	auto getFileStem() const noexcept { return mFilePath.stem().string(); }
	auto getFileExts() const noexcept { return mFilePath.extension().string(); }
	auto getFileSpan() const noexcept { return std::span{ mFileData }; }
	auto getFileSize() const noexcept { return mFileData.size(); }
	auto getFileData() const noexcept { return mFileData.data(); }
	auto getFileSHA1() const noexcept { return mFileSHA1; }

	void setValidator(GameValidator func) noexcept { checkGame = func; }

	void clearCachedFileData() noexcept;
	bool validateGameFile(const Path) noexcept;
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
