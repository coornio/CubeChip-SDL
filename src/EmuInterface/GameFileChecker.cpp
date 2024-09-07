/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

//#include <memory>
//#include <iostream>

#include "../_nlohmann/json.hpp"
using json = nlohmann::json;

#include "../Assistants/BasicLogger.hpp"

#include "GameFileChecker.hpp"


#include "CHIP8/Cores/CHIP8_MODERN.hpp"

/*==================================================================*/

GameCoreType   GameFileChecker::sEmuCore{};
nlohmann::json GameFileChecker::sEmuConfig{};

/*==================================================================*/

void GameFileChecker::deleteGameCore() noexcept {
	sEmuConfig.clear();
	sEmuCore = GameCoreType::INVALID;
}

auto GameFileChecker::getGameCoreType() noexcept { return sEmuCore; }

bool GameFileChecker::hasGameCoreType() noexcept { return sEmuCore != GameCoreType::INVALID; }

/*==================================================================*/

std::unique_ptr<EmuInterface> GameFileChecker::constructCore() {
	try {
		switch (sEmuCore) {
			case GameCoreType::XOCHIP:
				//return std::make_unique<XOCHIP>();

			case GameCoreType::CHIP8E:
				//return std::make_unique<CHIP8E>();

			case GameCoreType::CHIP8X:
				//return std::make_unique<CHIP8X>();

			case GameCoreType::CHIP8_2P:
				//return std::make_unique<CHIP8_2P>();

			case GameCoreType::CHIP8_4P:
				//return std::make_unique<CHIP8_4P>();

			case GameCoreType::CHIP8_LEGACY:
				//return std::make_unique<CHIP8_LEGACY>();

			case GameCoreType::SCHIP_LEGACY:
				//return std::make_unique<SCHIP_LEGACY>();

			case GameCoreType::CHIP8_MODERN:
				return std::make_unique<CHIP8_MODERN>();

			case GameCoreType::SCHIP_MODERN:
				//return std::make_unique<SCHIP_MODERN>();

			case GameCoreType::CHIP8X_HIRES:
				//return std::make_unique<CHIP8X_HIRES>();

			case GameCoreType::CHIP8X_SCHIP:
				//return std::make_unique<CHIP8X_SCHIP>();

			case GameCoreType::HWCHIP64:
				//return std::make_unique<HWCHIP64>();

			case GameCoreType::MEGACHIP:
				//return std::make_unique<MEGACHIP>();

			case GameCoreType::GIGACHIP:
				//return std::make_unique<GIGACHIP>();

			default:
			case GameCoreType::INVALID:
				return nullptr;
		}
	} catch (const std::exception&) {
		blog.newEntry(BLOG::ERROR, "Failed to allocate memory for the Game Core!");
		return nullptr;
	}
}

std::unique_ptr<EmuInterface> GameFileChecker::initGameCore() noexcept {
	auto tempCore{ constructCore() };

	if (tempCore) {
		if (tempCore->isCoreStopped()) {
			if (hasGameCoreType()) {
				blog.newEntry(BLOG::ERROR, "Failed critical Game Core initialization requirements!");
				deleteGameCore();
			}
			return nullptr;
		} else {
			return tempCore;
		}
	} else {
		return nullptr;
	}
}

/*==================================================================*/

bool GameFileChecker::validate(
	const std::size_t  size,
	const std::string& type,
	const std::string& sha1
) noexcept {
	if (sha1.empty()) {
		return validate(size, type);
	} else {
		/* database check here */
		return validate(size, type); // placeholder
	}
}

bool GameFileChecker::validate(
	const std::size_t  size,
	const std::string& type
) noexcept {
	static const std::unordered_map <std::string_view, GameFileType> sExtMap{
		{".c2x", GameFileType::c2x},
		{".c4x", GameFileType::c4x},
		{".c8x", GameFileType::c8x},
		{".c8e", GameFileType::c8e},
		{".c2h", GameFileType::c2h},
		{".c4h", GameFileType::c4h},
		{".c8h", GameFileType::c8h},
		{".ch8", GameFileType::ch8},
		{".sc8", GameFileType::sc8},
		{".mc8", GameFileType::mc8},
		{".gc8", GameFileType::gc8},
		{".xo8", GameFileType::xo8},
		{".hwc", GameFileType::hwc},
		{".bnc", GameFileType::bnc},
	};

	const auto it{ sExtMap.find(type) };
	if (it == sExtMap.end()) {
		blog.newEntry(BLOG::WARN, "Cannot match Game to a supported system/platform!");
		return false;
	}

	switch (it->second) {
		case (GameFileType::c2x):
		case (GameFileType::c4x):
			return testGame(
				true,
				//CHIP8X_HIRES::testGameSize(size),
				GameCoreType::CHIP8X_HIRES
			);

		case (GameFileType::c8x):
			return testGame(
				true,
				//CHIP8X::testGameSize(size),
				GameCoreType::CHIP8X
			);

		case (GameFileType::c2h):
			return testGame(
				true,
				//CHIP8_2P::testGameSize(size),
				GameCoreType::CHIP8_2P
			);

		case (GameFileType::c4h):
			return testGame(
				true,
				//CHIP8_4P::testGameSize(size),
				GameCoreType::CHIP8_4P
			);

		case (GameFileType::mc8):
			return testGame(
				true,
				//MEGACHIP::testGameSize(size),
				GameCoreType::MEGACHIP
			);

		case (GameFileType::gc8):
			return testGame(
				true,
				//GIGACHIP::testGameSize(size),
				GameCoreType::GIGACHIP
			);

		case (GameFileType::xo8):
			return testGame(
				true,
				//XOCHIP::testGameSize(size),
				GameCoreType::XOCHIP
			);

		case (GameFileType::hwc):
			return testGame(
				true,
				//HWCHIP64::testGameSize(size),
				GameCoreType::HWCHIP64
			);

		case (GameFileType::c8e):
			return testGame(
				true,
				//CHIP8E::testGameSize(size),
				GameCoreType::CHIP8E
			);

		case (GameFileType::c8h):
			return testGame(
				true,
				//CHIP8_2P::testGameSize(size),
				GameCoreType::CHIP8_2P
			); // patched!

		case (GameFileType::ch8):
			return testGame(
				CHIP8_MODERN::testGameSize(size),
				GameCoreType::CHIP8_MODERN
			);

		case (GameFileType::sc8):
			return testGame(
				true,
				//SCHIP_MODERN::testGameSize(size),
				GameCoreType::SCHIP_MODERN
			);

		case (GameFileType::bnc):
			return testGame(
				CHIP8_MODERN::testGameSize(size),
				GameCoreType::CHIP8_MODERN
			);
	}
	return false;
};

