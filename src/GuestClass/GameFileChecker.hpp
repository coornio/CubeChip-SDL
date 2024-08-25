/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <utility>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

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

class EmuCores;

class HomeDirManager;
class BasicVideoSpec;
class BasicAudioSpec;

class GameFileChecker final {
	 GameFileChecker() = delete;
	~GameFileChecker() = delete;

	static std::string sErrorMsg;
	static GameCoreType sEmuCore;

	[[nodiscard]] static constexpr bool testGame(
		const bool pass, const GameCoreType type
	) noexcept {
		if (pass) { sEmuCore = type; }
		return pass;
	}

public:
	[[nodiscard]] static auto getError() noexcept {
		return std::move(sErrorMsg);
	}

	[[nodiscard]] static auto getCore()  noexcept {
		return sEmuCore;
	}

	static void delCore() noexcept {
		sErrorMsg.clear();
		sEmuCore = GameCoreType::INVALID;
	}

	[[nodiscard]] static bool hasCore() noexcept {
		return sEmuCore != GameCoreType::INVALID;
	}

	[[nodiscard]] static std::unique_ptr<EmuCores> initializeCore(
		HomeDirManager&, BasicVideoSpec&, BasicAudioSpec&
	);

	static bool validate(
		std::uint64_t    size,
		std::string_view type,
		std::string_view sha1 = ""
	) noexcept;
};
