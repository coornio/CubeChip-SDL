/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

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

class CoreRegistry {
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

	using CoreRegList = std::vector<CoreDetails>;
	static inline std::unordered_map<Str, CoreRegList> mRegistry{};
	static inline CoreRegList mEligible{};
	static inline CoreDetails mCurrentCore{};

	CoreRegistry()                               = delete;
	CoreRegistry(const CoreRegistry&)            = delete;
	CoreRegistry& operator=(const CoreRegistry&) = delete;

public:
	static bool validateProgram(const char* fileData, ust fileSize, const Str& fileType, const Str& fileSHA1) noexcept {
		if (fileSHA1.empty()) {
			/* fall back to extension-based core pick */
			return validateProgram(fileData, fileSize, fileType);
		} else {
			/* decide based on database entry instead */
			return validateProgram(fileData, fileSize, fileType); // placeholder
		}
	}

private:
	static bool validateProgram(const char* fileData, ust fileSize, const Str& fileType) noexcept;

public:
	static void registerCore(CoreConstructor&& ctor, ProgramTester&& tester, FileExtList exts) {
		CoreDetails reg{ ctor, tester, std::move(exts), };
		for (const auto& ext : reg.fileExtensions) {
			mRegistry[ext].push_back(reg);
		}
	}

	static const CoreRegList* findCore(const Str& ext) {
		auto it = mRegistry.find(ext);
		return it != mRegistry.end() ? &it->second : nullptr;
	}

	[[nodiscard]]
	static EmuInterface* constructCore() noexcept;

	static void clearEligibleCores() noexcept {
		mEligible.clear();
		mCurrentCore.clear();
	}
	static void clearCurrentCore() noexcept {
		mCurrentCore.clear();
	}

	[[nodiscard]]
	static const auto& getEligibleCores() noexcept { return mEligible; }
	[[nodiscard]]
	static const auto& getCurrentCore() noexcept { return mCurrentCore; }

	template <typename Core>
		requires (std::convertible_to<Core*, EmuInterface*>)
	struct Register {
		Register(ProgramTester&& tester, FileExtList exts) {
			registerCore([]() -> EmuInterface* { return new Core(); },
				std::move(tester), std::move(exts));
		}
	};
};
