/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <vector>
#include <unordered_map>

#include "../Assistants/Typedefs.hpp"
#include "../Libraries/nlohmann/json_fwd.hpp"

/*==================================================================*/

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
	bnc,
	BytePusher,
	gb, gbc // GAMEBOY/GAMEBOY COLOR
};

/*==================================================================*/

class EmuInterface;

using CoreConstructor = EmuInterface* (*)();
using ProgramTester   = bool (*)(const char*, ust);
using FileExtList     = std::vector<Str>;

struct CoreDetails {
	CoreConstructor constructCore{};
	ProgramTester   testProgram{};
	FileExtList     fileExtensions{};

	void clear() noexcept {
		constructCore = nullptr;
		testProgram   = nullptr;
		fileExtensions.clear();
	}
};

using CoreRegList  = std::vector<CoreDetails>;
using JsonDatabase = nlohmann::json;

/*==================================================================*/

class CoreRegistry {
	using Registrations = std::unordered_map<Str, CoreRegList>;
	static inline Registrations sRegistry{};
	static inline CoreRegList   sEligible{};
	static inline CoreDetails   sCurrentCore{};
	static        JsonDatabase  sProgramDB;
	static        JsonDatabase  sCoreConfig;

	CoreRegistry()                               = delete;
	CoreRegistry(const CoreRegistry&)            = delete;
	CoreRegistry& operator=(const CoreRegistry&) = delete;

	static bool validateProgramByHash(const char* fileData, ust fileSize, const Str& fileSHA1) noexcept;
	static bool validateProgramByType(const char* fileData, ust fileSize, const Str& fileType) noexcept;

public:
	static bool validateProgram(const char* fileData, ust fileSize, const Str& fileType, const Str& fileSHA1) noexcept;

	static void loadProgramDB(const Path& dbPath = {}) noexcept;

	/*==================================================================*/

	static void registerCore(CoreConstructor&& ctor, ProgramTester&& tester, FileExtList exts) noexcept;

	static const CoreRegList* findEligibleCores(const Str& ext) noexcept;

	[[nodiscard]]
	static EmuInterface* constructCore(size_type idx = 0) noexcept;

	/*==================================================================*/

	static void clearEligibleCores() noexcept {
		sEligible.clear();
		sCurrentCore.clear();
	}
	static void clearCurrentCore() noexcept {
		sCurrentCore.clear();
	}

	[[nodiscard]]
	static const auto& getEligibleCores() noexcept { return sEligible; }
	[[nodiscard]]
	static const auto& getCurrentCore() noexcept { return sCurrentCore; }

	/*==================================================================*/

	template <typename Core>
		requires (std::convertible_to<Core*, EmuInterface*>)
	struct Register {
		Register(ProgramTester&& tester, FileExtList exts) {
			registerCore([]() -> EmuInterface* { return new Core(); },
				std::move(tester), std::move(exts));
		}
	};
};
