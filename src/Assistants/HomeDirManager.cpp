/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "HomeDirManager.hpp"

#include "SHA1.hpp"
#include "SimpleFileIO.hpp"
#include "BasicLogger.hpp"
#include "DefaultConfig.hpp"
#include "PathGetters.hpp"

#include <SDL3/SDL_messagebox.h>

/*==================================================================*/
	#pragma region HomeDirManager Class

HomeDirManager::HomeDirManager(
	const char* override, const char* config,
	const char* org,      const char* app
) noexcept {
	if (!setPortable(override))
		{ /* nothing here yet */ }

	if (!setHomePath(org, app))
		{ mSuccessful = false; return; }

	blog.initLogFile("program.log", sHomePath);

	if (!config) { config = "settings.toml"; }
	static const auto mainConfig{ (Path{ sHomePath } / config).string() };
	sConfPath = mainConfig.c_str();
}

bool HomeDirManager::setPortable(const char* override) noexcept {
	if (override) {
		sHomePath = override;
		sPortable = true;
		return sPortable;
	}
	if (!::getBasePath()) {
		sPortable = false;
		return sPortable;
	}

	const auto fileExists{ fs::exists(Path{ ::getBasePath() } / "portable.txt") };
	if (!fileExists || !fileExists.value()) {
		sPortable = false;
		return sPortable;
	} else {
		sHomePath = ::getBasePath();
		sPortable = true;
		return sPortable;
	}
}

bool HomeDirManager::setHomePath(const char* org, const char* app) noexcept {
	if (sHomePath) { return true; }
	else { ::getHomePath(org, app); }

	if (sPortable) {
		if (!sHomePath) // something went wrong
			{ goto fallback; }
		else { return true; }
	} else {
		fallback:
		sHomePath = ::getHomePath();
		sPortable = false;
		return sHomePath;
	}
}

void HomeDirManager::parseMainAppConfig() const noexcept {
	if (const auto result{ config::parseFromFile(sConfPath) }) {
		config::safeTableUpdate(sMainAppConfig, result.table());
		blog.newEntry(BLOG::INFO,
			"[TOML] App Config found, previous settings loaded!");
	} else {
		blog.newEntry(BLOG::WARN,
			"[TOML] App Config failed to load! [{}]", result.error().description());
	}
}

void HomeDirManager::writeMainAppConfig() const noexcept {
	if (const auto result{ config::writeToFile(sMainAppConfig, sConfPath) }) {
		blog.newEntry(BLOG::INFO,
			"[TOML] App Config written to file successfully!");
	} else {
		blog.newEntry(BLOG::ERROR,
			"[TOML] Failed to write App Config, runtime settings lost! [{}]", result.error().message());
	}
}

void HomeDirManager::insertIntoMainAppConfig(const SettingsMap& map) const noexcept {
	for (auto const& pair : map) {
		const auto& key{ pair.first };
		const auto& ref{ pair.second };
		ref.visit([&](auto* ptr) { config::set(sMainAppConfig, key, *ptr); });
	}
}

void HomeDirManager::updateFromMainAppConfig(const SettingsMap& map) const noexcept {
	for (auto const& pair : map) {
		const auto& key{ pair.first };
		const auto& ref{ pair.second };
		ref.visit([&](auto* ptr) { config::get(sMainAppConfig, key, *ptr); });
	}
}

HomeDirManager* HomeDirManager::initialize(
	const char* override, const char* config,
	const char* org,      const char* app
) noexcept {
	/*pass settings filename here, and cxx params*/
	static HomeDirManager self(override, config, org, app);
	return mSuccessful ? &self : nullptr;
}

Path* HomeDirManager::addSystemDir(const Path& sub, const Path& sys) noexcept {
	if (sub.empty()) { return nullptr; }
	
	const auto newDirPath{ sHomePath / sys / sub };

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
