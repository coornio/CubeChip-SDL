/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../Assistants/FrameLimiter.hpp"
#include "../Assistants/BasicInput.hpp"
#include "../Assistants/Well512.hpp"

#include "EmuInterface.hpp"

/*==================================================================*/

void EmuInterface::startWorker() noexcept {
	if (mCoreThread.joinable()) { return; }
	mCoreThread = std::jthread([this](std::stop_token token) { threadEntry(token); });
}

void EmuInterface::stopWorker() noexcept {
	if (mCoreThread.joinable()) {
		mCoreThread.request_stop();
		mCoreThread.join();
	}
}

void EmuInterface::threadEntry(std::stop_token token) {
	while (!token.stop_requested())
		[[likely]] { mainSystemLoop(); }
}

EmuInterface::EmuInterface() noexcept
	: Pacer{ std::make_unique<FrameLimiter>() }
	, Input{ std::make_unique<BasicKeyboard>() }
{
	static Well512 sWell512;
	RNG = &sWell512;
}

EmuInterface::~EmuInterface() noexcept {}

/*==================================================================*/

void EmuInterface::setSystemFramerate(f32 value) noexcept {
	mTargetFPS = value;
	Pacer->setLimiter(value);
}

void EmuInterface::writeStatistics() {
	if (Pacer->getValidFrameCounter() & 0x1) [[likely]] {
		mStatistics.store(std::make_shared<Str>(std::format(
			"Time Since:{:9.3f} ms\n"
			"Frame Work:{:9.3f} ms\n",
			Pacer->getElapsedMillisLast(),
			Pacer->getElapsedMicrosSince() / 1000.0f
		)), mo::release);
	}
}

Str EmuInterface::fetchStatistics() const noexcept {
	const auto string{ mStatistics.load(mo::acquire) };
	return string ? *string : std::string{};
}
