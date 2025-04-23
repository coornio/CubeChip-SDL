/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <span>
#include <vector>
#include <algorithm>

#include "Misc.hpp"
#include "SettingWrapper.hpp"

#define TOML_EXCEPTIONS 0
#include "../Libraries/toml++/toml.hpp"

/*==================================================================*/
	#pragma region HomeDirManager Singleton Class

class HomeDirManager final {
	HomeDirManager(
		const char* override, const char* config,
		const char* org,      const char* app
	) noexcept;
	HomeDirManager(const HomeDirManager&) = delete;
	HomeDirManager& operator=(const HomeDirManager&) = delete;

	using GameValidator = bool (*)(
		const char* fileData,
		const ust   fileSize,
		const Str&  fileExts,
		const Str&  fileSHA1
	) noexcept;

	Path mFilePath{};
	Str  mFileSHA1{};

	std::vector<char>
		mFileData{};

	std::vector<Path>
		mDirectories{};

	GameValidator checkGame{};

public:
	inline static toml::table sMainAppConfig{};

private:
	inline static const char* sHomePath{};
	inline static const char* sConfPath{};
	inline static bool sPortable{};

	static inline bool mSuccessful{ true };

	bool setPortable(const char* override) noexcept;
	bool setHomePath(const char* org, const char* app) noexcept;

public:
	void parseMainAppConfig() const noexcept;

	template <typename... Maps>
		requires (std::same_as<Maps, SettingsMap> && ...)
	void parseMainAppConfig(const Maps&... maps) const noexcept {
		(insertIntoMainAppConfig(maps), ...);
		parseMainAppConfig();
		(updateFromMainAppConfig(maps), ...);
	}

	void writeMainAppConfig() const noexcept;

	template <typename... Maps>
		requires (std::same_as<Maps, SettingsMap> && ...)
	void writeMainAppConfig(const Maps&... maps) const noexcept {
		(insertIntoMainAppConfig(maps), ...);
		writeMainAppConfig();
	}

private:
	void insertIntoMainAppConfig(const SettingsMap& map) const noexcept;
	void updateFromMainAppConfig(const SettingsMap& map) const noexcept;

public:
	static HomeDirManager* initialize(
		const char* override, const char* config,
		const char* org,      const char* app
	) noexcept;
	static bool isSuccessful() noexcept { return mSuccessful; }

	Path* addSystemDir(const Path& sub, const Path& sys = Path{}) noexcept;

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
	bool validateGameFile(const Path& gamePath) noexcept;
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
