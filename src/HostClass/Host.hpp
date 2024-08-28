/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <memory>
#include <mutex>
#include <filesystem>

#include <SDL3/SDL_init.h>

class HomeDirManager;
class BasicVideoSpec;
class BasicAudioSpec;

class FrameLimiter;
class EmuInterface;

/*==================================================================*/
	#pragma region VM_Host Singleton Class
/*==================================================================*/

class VM_Host final {

	using FilePath = std::filesystem::path;

	std::unique_ptr<EmuInterface> iGuest;
	std::unique_ptr<FrameLimiter> Limiter;

	std::unique_ptr<HomeDirManager> HDM;
	std::unique_ptr<BasicVideoSpec> BVS;
	std::unique_ptr<BasicAudioSpec> BAS;

	bool runBenchmark{};
	bool forceFileCfg{};

	bool errorTriggered{};

	bool initGameCore() noexcept;
	void replaceGuest(const bool);

	VM_Host(const FilePath&) noexcept;
	VM_Host(const VM_Host&) = delete;
	VM_Host& operator=(const VM_Host&) = delete;

public:
	std::mutex Mutex;

	bool getSelfStatus() const noexcept { return errorTriggered; }

	static VM_Host* initialize(const FilePath&) noexcept;

	void pauseSystem(const bool state) const noexcept;
	void loadGameFile(const FilePath&, const bool = false);

	SDL_AppResult runFrame();
};

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
