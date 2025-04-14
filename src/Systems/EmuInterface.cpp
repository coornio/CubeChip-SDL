/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../Assistants/ThreadAffinity.hpp"
#include "../Assistants/BasicVideoSpec.hpp"
#include "../Assistants/FrameLimiter.hpp"
#include "../Assistants/BasicInput.hpp"
#include "../Assistants/Well512.hpp"

#include "EmuInterface.hpp"

/*==================================================================*/

void EmuInterface::startWorker() noexcept {
	if (mCoreThread.joinable()) { return; }
	mCoreThread = Thread([this](StopToken token) { threadEntry(token); });
}

void EmuInterface::stopWorker() noexcept {
	if (mCoreThread.joinable()) {
		mCoreThread.request_stop();
		mCoreThread.join();
	}
}

void EmuInterface::threadEntry(StopToken token) {
	thread_affinity::set_affinity(~0b11ull);
	SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_HIGH);
	
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

void EmuInterface::setViewportSizes(s32 texture_W, s32 texture_H, s32 upscale_M, s32 padding_S) noexcept {
	BVS->setViewportSizes(texture_W, texture_H, upscale_M, padding_S);
}

void EmuInterface::setDisplayBorderColor(u32 color) noexcept {
	BVS->setBorderColor(color);
}

void EmuInterface::setSystemFramerate(f32 value) noexcept {
	mTargetFPS = value;
	Pacer->setLimiter(value);
}

void EmuInterface::writeStatistics() {
	if (Pacer->getValidFrameCounter() & 0x1) [[likely]] {
		mStatistics.store(std::make_shared<Str>(fmt::format(
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
