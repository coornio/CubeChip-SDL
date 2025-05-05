/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include <shared_mutex>

#include "Assistants/Typedefs.hpp"

#include "Libraries/cxxopts/cxxopts.hpp"

/*==================================================================*/

constexpr auto* AppName{ "CubeChip" };
constexpr auto* AppVer{ __DATE__ };

/*==================================================================*/

class HomeDirManager;
class BasicAudioSpec;
class BasicVideoSpec;

class EmuInterface;

union SDL_Event;

/*==================================================================*/
	#pragma region EmuHost Singleton Class

class EmuHost final {
	EmuHost(const Path&) noexcept;

	EmuHost(const EmuHost&) = delete;
	EmuHost& operator=(const EmuHost&) = delete;

	struct StopEmuCoreThread {
		void operator()(EmuInterface* ptr) noexcept;
	};

	std::unique_ptr<
		EmuInterface,
		StopEmuCoreThread
	> iGuest;

public:
	static inline HomeDirManager* HDM{};
	static inline BasicAudioSpec* BAS{};
	static inline BasicVideoSpec* BVS{};

	std::shared_mutex Mutex;

private:
	bool mFrameStat{};
	bool mUnlimited{};

	void checkForHotkeys();
	void toggleSystemLimiter() noexcept;

	void discardCore();
	void replaceCore();

public:
	static auto* initialize(const Path& gamePath) noexcept {
		static EmuHost self(gamePath);
		return &self;
	}

	static bool initApplication(
		StrV overrideHome, StrV configName,
		bool forcePortable, StrV org, StrV app
	) noexcept;

	s32  processEvents(SDL_Event* event) noexcept;

	void hideMainWindow(bool state) noexcept;
	void pauseSystem(bool state) noexcept;
	void quitApplication() noexcept;
	void loadGameFile(const Path&);

	void processFrame();
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
