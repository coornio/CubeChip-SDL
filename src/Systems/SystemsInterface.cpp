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

#include "SystemsInterface.hpp"

/*==================================================================*/

void SystemsInterface::startWorker() noexcept {
	if (mCoreThread.joinable()) { return; }
	mCoreThread = Thread([this](StopToken token) { threadEntry(token); });
}

void SystemsInterface::stopWorker() noexcept {
	if (mCoreThread.joinable()) {
		mCoreThread.request_stop();
		mCoreThread.join();
	}
}

void SystemsInterface::threadEntry(StopToken token) {
	thread_affinity::set_affinity(~0b11ull);
	SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_HIGH);
	
	while (!token.stop_requested())
		[[likely]] { mainSystemLoop(); }
}

SystemsInterface::SystemsInterface() noexcept
	: mOverlayData{ std::make_shared<Str>() }
	, Input{ std::make_unique<BasicKeyboard>() }
	, Pacer{ std::make_unique<FrameLimiter>() }
{
	static Well512 sWell512;
	RNG = &sWell512;
}

SystemsInterface::~SystemsInterface() noexcept {}

/*==================================================================*/

void SystemsInterface::setViewportSizes(s32 texture_W, s32 texture_H, s32 upscale_M, s32 padding_S) noexcept {
	BVS->setViewportSizes(texture_W, texture_H, upscale_M, padding_S);
}

void SystemsInterface::setDisplayBorderColor(u32 color) noexcept {
	BVS->setBorderColor(color);
}

f32 SystemsInterface::getSystemFramerate() const noexcept {
	return mTargetFPS.load(mo::relaxed);
}

void SystemsInterface::setSystemFramerate(f32 value) noexcept {
	mTargetFPS.store(value, mo::relaxed);
	Pacer->setLimiter(value);
}

void SystemsInterface::saveOverlayData(const char* data) {
	mOverlayData.store(std::make_shared<Str>(data), mo::release);
}

Str SystemsInterface::makeOverlayData() {
	const auto frameMS{ Pacer->getElapsedMillisLast() };
	const auto elapsed{ Pacer->getElapsedMicrosSince() / 1000.0f };

	return fmt::format(
		"Framerate:{:9.3f} fps |{:9.3f}ms\n"
		"Frametime:{:9.3f} ms ({:3.2f}%)\n",
		frameMS < Epsilon::f32 ? getSystemFramerate()
			: std::round(1000.0f / frameMS * 100.0f) / 100.0f,
		frameMS, elapsed, elapsed / Pacer->getFramespan() * 100.0f
	);
}

void SystemsInterface::pushOverlayData() {
	if (Pacer->getValidFrameCounter() & 0x1) [[likely]] {
		saveOverlayData(SystemsInterface::makeOverlayData().c_str());
	}
}

Str SystemsInterface::copyOverlayData() const noexcept {
	return *mOverlayData.load(mo::acquire);
}
