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
	bool _initFailure{};

	std::unique_ptr<HomeDirManager> HDM;
	std::unique_ptr<BasicVideoSpec> BVS;
	std::unique_ptr<BasicAudioSpec> BAS;
	std::unique_ptr<VM_Guest>       Guest;

	[[nodiscard]] bool isReady() const;
	[[nodiscard]] bool doBench() const;
	void isReady(bool);
	void doBench(bool);

	void prepareGuest(FrameLimiter&);
	bool mainHostLoop(FrameLimiter&, SDL_Event&);
	bool eventLoopSDL(FrameLimiter&, SDL_Event&);

public:
	explicit VM_Host(const char* const);
	~VM_Host();

	bool runHost();

	[[nodiscard]]
	bool initFailure() const { return _initFailure; }
};
