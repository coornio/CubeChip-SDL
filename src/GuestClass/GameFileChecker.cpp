/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "GameFileChecker.hpp"

#pragma warning(push)
	#pragma warning(disable : 26819) // C fallthrough warning disabled
	#include "../_nlohmann/json.hpp"
#pragma warning(pop)

#include "EmuCores/CHIP8_MODERN.hpp"

std::string GameFileChecker::sErrorMsg{};
GameCoreType GameFileChecker::sEmuCore{};

bool GameFileChecker::validate(
	const std::uint64_t    size,
	const std::string_view type,
	const std::string_view sha1
) {
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

	if (!sha1.empty()) {
		/* database check here */
	}

	const auto it{ sExtMap.find(type) };
	if (it == sExtMap.end()) {
		sErrorMsg = "unknown filetype or platform";
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
