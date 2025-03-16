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

class spin_mutex {
	std::atomic_flag m_f{};

public:
	void lock() {
		while (m_f.test_and_set())
			{ m_f.wait(true); }
	}
	void unlock() {
		m_f.clear();
		m_f.notify_one();
	}
};

/*==================================================================*/

class HomeDirManager;
class BasicVideoSpec;
class BasicAudioSpec;

class EmuInterface;

/*==================================================================*/
	#pragma region EmuHost Singleton Class

class EmuHost final {
	EmuHost(const Path&) noexcept;
	~EmuHost() noexcept;

	EmuHost(const EmuHost&) = delete;
	EmuHost& operator=(const EmuHost&) = delete;

	static inline HomeDirManager* HDM{};
	static inline BasicVideoSpec* BVS{};
	static inline BasicAudioSpec* BAS{};

	struct StopEmuCoreThread {
		void operator()(EmuInterface* ptr) const noexcept;
	};

	std::unique_ptr<EmuInterface, StopEmuCoreThread> iGuest;

public:
	std::shared_mutex Mutex;

private:
	bool mFrameStat{};
	bool mUnlimited{};

	void checkForHotkeys();

	void discardCore();
	void replaceCore();

public:
	static auto* create(const Path& gamePath) noexcept {
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
		return HDM && BVS && BAS;
	}

	bool isMainWindow(u32 windowID) const noexcept;

	template <typename T>
	void scaleInterface(T&& appFont) const noexcept {
		BVS->scaleInterface(std::forward<T>(appFont));
	}

	template <typename T>
	void processInterfaceEvent(T&& event) const noexcept {
		BVS->processInterfaceEvent(std::forward<T>(event));
	}

	void pauseSystem(bool state) const noexcept;
	void quitApplication() noexcept;
	void loadGameFile(const Path&);

	void processFrame();
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
