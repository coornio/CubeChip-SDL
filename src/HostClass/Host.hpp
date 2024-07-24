/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <memory>

class HomeDirManager;
class BasicVideoSpec;
class BasicAudioSpec;

class FrameLimiter;
class VM_Guest;

class VM_Host final {
	bool _isReady{};
	bool _doBench{};

	HomeDirManager& HDM;
	BasicVideoSpec& BVS;
	BasicAudioSpec& BAS;

	std::unique_ptr<VM_Guest> Guest;

	[[nodiscard]] bool isReady() const;
	[[nodiscard]] bool doBench() const;
	void isReady(bool);
	void doBench(bool);

	void prepareGuest(FrameLimiter&);
	bool mainHostLoop(FrameLimiter&, SDL_Event&);
	bool eventLoopSDL(FrameLimiter&, SDL_Event&);

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
