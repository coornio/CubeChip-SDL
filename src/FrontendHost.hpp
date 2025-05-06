/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <memory>
#include <atomic>

#include "Assistants/Typedefs.hpp"

#include "Libraries/cxxopts/cxxopts.hpp"

/*==================================================================*/

#if !defined(NDEBUG) || defined(DEBUG)
	constexpr auto* AppName{ "[DEBUG] CubeChip" };
#else
	constexpr auto* AppName{ "CubeChip" };
#endif

constexpr auto* AppVer{ __DATE__ };

/*==================================================================*/

class HomeDirManager;
class BasicAudioSpec;
class BasicVideoSpec;

class SystemsInterface;

union SDL_Event;

/*==================================================================*/

class FrontendHost final {
	FrontendHost(const Path&) noexcept;

	FrontendHost(const FrontendHost&) = delete;
	FrontendHost& operator=(const FrontendHost&) = delete;

	struct StopEmuCoreThread {
		void operator()(SystemsInterface* ptr) noexcept;
	};

	std::unique_ptr<
		SystemsInterface,
		StopEmuCoreThread
	> mSystemCore;

public:
	static inline HomeDirManager* HDM{};
	static inline BasicAudioSpec* BAS{};
	static inline BasicVideoSpec* BVS{};

private:
	bool mFrameStat{};
	bool mUnlimited{};

	void checkForHotkeys();
	void toggleSystemLimiter() noexcept;

	void discardCore();
	void replaceCore();

public:
	static auto* initialize(const Path& gamePath) noexcept {
		static FrontendHost self(gamePath);
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

/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
