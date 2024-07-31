/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <atomic>
#include <thread>
#include <stop_token>

class HomeDirManager;
class BasicVideoSpec;
class BasicAudioSpec;

union SDL_Event;

class alignas(64) VM_Host final {
	std::atomic<bool> _isReadyToEmulate{};
	std::atomic<bool> _testBenchmarking{};

	HomeDirManager& HDM;
	BasicVideoSpec& BVS;
	BasicAudioSpec& BAS;

	std::jthread workerGuest;

	[[nodiscard]] bool isReadyToEmulate() const;
	[[nodiscard]] bool testBenchmarking() const;
	void isReadyToEmulate(bool);
	void testBenchmarking(bool);

	void executeWorker(std::stop_token);
	void disableWorker();
	void prepareWorker();

public:
	explicit VM_Host(
		const char* const,
		HomeDirManager&,
		BasicVideoSpec&,
		BasicAudioSpec&
	);
	~VM_Host();

	bool runHost();
};
