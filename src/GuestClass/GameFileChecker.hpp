/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

#include "../_nlohmann/json_fwd.hpp"

enum class GameFileType {
	c2x, // CHIP-8X 2-page
	c4x, // CHIP-8X 4-page
	c8x, // CHIP-8X
	c8e, // CHIP-8E
	c2h, // CHIP-8 (HIRES) 2-page
	c4h, // CHIP-8 (HIRES) 4-page
	c8h, // CHIP-8 (HIRES) 2-page patched
	ch8, // CHIP-8
	sc8, // SUPERCHIP
	mc8, // MEGACHIP
	gc8, // GIGACHIP
	xo8, // XO-CHIP
	hwc, // HYPERWAVE-CHIP
	bnc, // benchmark
};

enum class GameCoreType {
	INVALID,
	XOCHIP, CHIP8E, CHIP8X,
	CHIP8_2P, CHIP8_4P,
	CHIP8_LEGACY, SCHIP_LEGACY,
	CHIP8_MODERN, SCHIP_MODERN,
	CHIP8X_HIRES, CHIP8X_SCHIP,
	HWCHIP64, MEGACHIP, GIGACHIP,
};

class EmuInterface;

class GameFileChecker final {
	 GameFileChecker() = delete;
	~GameFileChecker() = delete;

	static GameCoreType   sEmuCore;
	static nlohmann::json sEmuConfig;

	[[nodiscard]]
	static bool testGame(
		const bool condition,
		const GameCoreType type
	) noexcept {
		if (condition) { sEmuCore = type; }
		return condition;
	}

	static bool validate(
		const std::size_t  size,
		const std::string& type
	) noexcept;

	[[nodiscard]]
	static std::unique_ptr<EmuInterface> constructCore() noexcept;

public:
	static bool validate(
		const std::size_t  size,
		const std::string& type,
		const std::string& sha1
	) noexcept;

	[[nodiscard]]
	static std::unique_ptr<EmuInterface> initGameCore() noexcept;

	static void deleteGameCore()  noexcept;

	[[nodiscard]]
	static auto getGameCoreType()  noexcept;

	[[nodiscard]]
	static bool hasGameCoreType()  noexcept;
};
