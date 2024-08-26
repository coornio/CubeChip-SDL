/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

class HomeDirManager;
class BasicVideoSpec;
class BasicAudioSpec;

class FrameLimiter;
class EmuInterface;

union SDL_Event;

class VM_Host final {

	std::unique_ptr<EmuInterface>
		iGuest;

	bool _doBench{};

	[[nodiscard]]
	bool doBench() const noexcept;
	void doBench(bool) noexcept;

	bool initGameCore();

public:
	HomeDirManager& HDM;
	BasicVideoSpec& BVS;
	BasicAudioSpec& BAS;

	explicit VM_Host(
		const char* const,
		HomeDirManager&,
		BasicVideoSpec&,
		BasicAudioSpec&
	);
	~VM_Host();

	void prepareGuest(FrameLimiter&);
	bool handleEventSDL(FrameLimiter&, const SDL_Event*);
	bool runFrame(FrameLimiter& Limiter);
};
