/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <memory>
#include <mutex>
#include <filesystem>

class HomeDirManager;
class BasicVideoSpec;
class BasicAudioSpec;

class EmuInterface;
class FrameLimiter;

/*==================================================================*/
	#pragma region EmuHost Singleton Class
/*==================================================================*/

class EmuHost final {
	EmuHost(const std::filesystem::path&) noexcept;
	~EmuHost() noexcept;

	EmuHost(const EmuHost&) = delete;
	EmuHost& operator=(const EmuHost&) = delete;

	static HomeDirManager* HDM;
	static BasicVideoSpec* BVS;
	static BasicAudioSpec* BAS;

	std::unique_ptr<EmuInterface> iGuest;
	std::unique_ptr<FrameLimiter> Limiter;

public:
	std::mutex Mutex;

private:
	bool unlimitedMode{};

	void toggleUnlimited();
	void printStatistics() const;
	void checkForHotkeys();

	void discardCore();
	void replaceCore();

public:
	static auto* create(const std::filesystem::path& gamePath) noexcept {
		static EmuHost self(gamePath);
		return &self;
	}

	static bool assignComponents(
		HomeDirManager* const pHDM,
		BasicVideoSpec* const pBVS,
		BasicAudioSpec* const pBAS
	) noexcept {
		HDM = pHDM;
		BVS = pBVS;
		BAS = pBAS;
		return !(HDM && BVS && BAS);
	}

	void pauseSystem(const bool state) const noexcept;
	void loadGameFile(const std::filesystem::path&);

	void processFrame();
};

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
