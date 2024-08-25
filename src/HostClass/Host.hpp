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

class VM_Host final {

	std::unique_ptr<EmuInterface>
		iGuest;

	HomeDirManager& HDM;
	BasicVideoSpec& BVS;
	BasicAudioSpec& BAS;

	bool _doBench{};

	[[nodiscard]]
	bool doBench() const noexcept;
	void doBench(bool) noexcept;

	void prepareGuest(FrameLimiter&);
	bool eventLoopSDL(FrameLimiter&);

	bool initGameCore();

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
