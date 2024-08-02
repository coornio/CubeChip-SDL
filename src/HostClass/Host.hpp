/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <optional>

class HomeDirManager;
class BasicVideoSpec;
class BasicAudioSpec;

class FrameLimiter;
class VM_Guest;

class alignas(64) VM_Host final {
	bool _isReady{};
	bool _doBench{};

	HomeDirManager& HDM;
	BasicVideoSpec& BVS;
	BasicAudioSpec& BAS;

	[[nodiscard]] bool isReady() const;
	[[nodiscard]] bool doBench() const;
	void isReady(bool);
	void doBench(bool);

	void prepareGuest(std::optional<VM_Guest>&, FrameLimiter&);
	bool mainHostLoop(std::optional<VM_Guest>&, FrameLimiter&, SDL_Event&);
	bool eventLoopSDL(std::optional<VM_Guest>&, FrameLimiter&, SDL_Event&);

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
