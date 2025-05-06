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
	, mOverlayData{ std::make_shared<Str>() }
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

f32 EmuInterface::getSystemFramerate() const noexcept {
	return mTargetFPS.load(mo::relaxed);
}

void EmuInterface::setSystemFramerate(f32 value) noexcept {
	mTargetFPS.store(value, mo::relaxed);
	Pacer->setLimiter(value);
}

Str EmuInterface::makeOverlayData() {
	const auto elapsedMillis{ Pacer->getElapsedMillisLast() };

	return fmt::format(
		"Framerate:{:9.3f}\n"
		"Frametime:{:9.3f}ms |{:9.3f}ms\n",
		elapsedMillis < Epsilon::f32 ? getSystemFramerate()
			: std::round(1000.0f / elapsedMillis * 100.0f) / 100.0f,
		elapsedMillis, Pacer->getElapsedMicrosSince() / 1000.0f
	);
}

void EmuInterface::pushOverlayData() {
	if (Pacer->getValidFrameCounter() & 0x1) [[likely]] {
		mOverlayData.store(std::make_shared<Str> \
			(EmuInterface::makeOverlayData()), mo::release);
	}
}

Str EmuInterface::copyOverlayData() const noexcept {
	return *mOverlayData.load(mo::acquire);
}
