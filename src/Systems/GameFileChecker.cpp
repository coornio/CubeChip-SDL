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
#include "CHIP8/Cores/SCHIP_MODERN.hpp"
#include "CHIP8/Cores/SCHIP_LEGACY.hpp"
#include "CHIP8/Cores/MEGACHIP.hpp"
#include "CHIP8/Cores/XOCHIP.hpp"

#include "BYTEPUSHER/Cores/BYTEPUSHER_STANDARD.hpp"

#include "GAMEBOY/Cores/GAMEBOY_CLASSIC.hpp"

/*==================================================================*/

GameCoreType   GameFileChecker::sEmuCore{};
nlohmann::json GameFileChecker::sEmuConfig{};

/*==================================================================*/

void GameFileChecker::deleteGameCore() noexcept {
	sEmuConfig.clear();
	sEmuCore = GameCoreType::INVALID;
}

auto GameFileChecker::getGameCoreType() noexcept { return sEmuCore; }

bool GameFileChecker::isGameCoreValid() noexcept { return sEmuCore != GameCoreType::INVALID; }

/*==================================================================*/

std::unique_ptr<EmuInterface> GameFileChecker::constructCore() {
	try {
		switch (sEmuCore) {
			case GameCoreType::XOCHIP:
				return std::make_unique<XOCHIP>();

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
				return std::make_unique<SCHIP_LEGACY>();

			case GameCoreType::CHIP8_MODERN:
				return std::make_unique<CHIP8_MODERN>();

			case GameCoreType::SCHIP_MODERN:
				return std::make_unique<SCHIP_MODERN>();

			case GameCoreType::CHIP8X_HIRES:
				//return std::make_unique<CHIP8X_HIRES>();

			case GameCoreType::CHIP8X_SCHIP:
				//return std::make_unique<CHIP8X_SCHIP>();

			case GameCoreType::HWCHIP64:
				//return std::make_unique<HWCHIP64>();

			case GameCoreType::MEGACHIP:
				return std::make_unique<MEGACHIP>();

			case GameCoreType::GIGACHIP:
				//return std::make_unique<GIGACHIP>();

			case GameCoreType::BYTEPUSHER_STANDARD:
				return std::make_unique<BYTEPUSHER_STANDARD>();

			case GameCoreType::GAMEBOY_CLASSIC:
				return std::make_unique<GAMEBOY_CLASSIC>();

			case GameCoreType::GAMEBOY_COLOR:
				return std::make_unique<GAMEBOY_CLASSIC>();

			default:
			case GameCoreType::INVALID:
				return nullptr;
		}
	} catch (const std::exception&) {
		blog.newEntry(BLOG::ERROR, "Failed to construct Game Core!");
		return nullptr;
	}
}

std::unique_ptr<EmuInterface> GameFileChecker::initGameCore() noexcept {
	auto tempCore{ constructCore() };

	if (tempCore && isGameCoreValid()) {
		if (!tempCore->isCoreStopped()) { return tempCore; }
		blog.newEntry(BLOG::ERROR, "Failed to initialize Game Core!");
		deleteGameCore();
	}
	return nullptr;
}

/*==================================================================*/

bool GameFileChecker::validate(
	const char* fileData,
	const ust   fileSize,
	const Str&  fileType,
	const Str&  fileSHA1
) noexcept {
	if (fileSHA1.empty()) {
		return validate(fileData, fileSize, fileType);
	} else {
		/* database check here */
		return validate(fileData, fileSize, fileType); // placeholder
	}
}

bool GameFileChecker::validate(
	const char* fileData,
	const ust   fileSize,
	const Str&  fileType
) noexcept {
	static const std::unordered_map<StrV, GameFileType> sExtMap{
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
		{".BytePusher", GameFileType::BytePusher},
		{".gb",  GameFileType::gb},
		{".gbc", GameFileType::gbc},
	};

	const auto it{ sExtMap.find(fileType) };
	if (it == sExtMap.end()) {
		blog.newEntry(BLOG::WARN, "Cannot match Game to a supported system/platform!");
		return false;
	}

	switch (it->second) {
/*==================================================================*/
	#pragma region CHIP8 FILE EXTS

		case (GameFileType::c2x):
		case (GameFileType::c4x):
			return testGame(
				true,
				//CHIP8X_HIRES::isGameFileValid(fileData, fileSize),
				GameCoreType::CHIP8X_HIRES
			);

		case (GameFileType::c8x):
			return testGame(
				true,
				//CHIP8X::isGameFileValid(fileData, fileSize),
				GameCoreType::CHIP8X
			);

		case (GameFileType::c2h):
			return testGame(
				true,
				//CHIP8_2P::isGameFileValid(fileData, fileSize),
				GameCoreType::CHIP8_2P
			);

		case (GameFileType::c4h):
			return testGame(
				true,
				//CHIP8_4P::isGameFileValid(fileData, fileSize),
				GameCoreType::CHIP8_4P
			);

		case (GameFileType::mc8):
			return testGame(
				MEGACHIP::isGameFileValid(fileData, fileSize),
				GameCoreType::MEGACHIP
			);

		case (GameFileType::gc8):
			return testGame(
				true,
				//GIGACHIP::isGameFileValid(fileData, fileSize),
				GameCoreType::GIGACHIP
			);

		case (GameFileType::xo8):
			return testGame(
				XOCHIP::isGameFileValid(fileData, fileSize),
				GameCoreType::XOCHIP
			);

		case (GameFileType::hwc):
			return testGame(
				true,
				//HWCHIP64::isGameFileValid(fileData, fileSize),
				GameCoreType::HWCHIP64
			);

		case (GameFileType::c8e):
			return testGame(
				true,
				//CHIP8E::isGameFileValid(fileData, fileSize),
				GameCoreType::CHIP8E
			);

		case (GameFileType::c8h):
			return testGame(
				true,
				//CHIP8_2P::isGameFileValid(fileData, fileSize),
				GameCoreType::CHIP8_2P
			); // patched!

		case (GameFileType::bnc):
		case (GameFileType::ch8):
			return testGame(
				CHIP8_MODERN::isGameFileValid(fileData, fileSize),
				GameCoreType::CHIP8_MODERN
			);

		case (GameFileType::sc8):
			return testGame(
				SCHIP_LEGACY::isGameFileValid(fileData, fileSize),
				GameCoreType::SCHIP_LEGACY
			);

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region BYTEPUSHER FILE EXTS

		case (GameFileType::BytePusher):
			return testGame(
				BYTEPUSHER_STANDARD::isGameFileValid(fileData, fileSize),
				GameCoreType::BYTEPUSHER_STANDARD
			);

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

/*==================================================================*/
	#pragma region GAMEBOY FILE EXTS

		case (GameFileType::gb):
			return testGame(
				GAMEBOY_CLASSIC::isGameFileValid(fileData, fileSize),
				GameCoreType::GAMEBOY_CLASSIC
			);

		case (GameFileType::gbc):
			return testGame(
				GAMEBOY_CLASSIC::isGameFileValid(fileData, fileSize),
				GameCoreType::GAMEBOY_CLASSIC
			);

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
	}
	return false;
};

