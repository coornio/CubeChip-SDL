/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../Assistants/ThreadAffinity.hpp"
#include "../Assistants/BasicVideoSpec.hpp"

#include "SystemInterface.hpp"

/*==================================================================*/

void SystemInterface::startWorker() noexcept {
	if (mCoreThread.joinable()) { return; }
	mCoreThread = Thread([this](StopToken token) { threadEntry(token); });
}

void SystemInterface::stopWorker() noexcept {
	if (mCoreThread.joinable()) {
		mCoreThread.request_stop();
		mCoreThread.join();
	}
}

void SystemInterface::threadEntry(StopToken token) {
	thread_affinity::set_affinity(~0b11ull);
	SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_HIGH);

	while (!token.stop_requested())
		[[likely]] { mainSystemLoop(); }
}

SystemInterface::SystemInterface() noexcept
	: mOverlayData{ std::make_shared<Str>() }
{
	static thread_local auto pRNG  { std::make_unique<Well512>()       };
	static thread_local auto pPacer{ std::make_unique<FrameLimiter>()  };
	static thread_local auto pInput{ std::make_unique<BasicKeyboard>() };
	RNG   = pRNG.get();
	Pacer = pPacer.get();
	Input = pInput.get();
}

/*==================================================================*/

void SystemInterface::setViewportSizes(bool cond, u32 W, u32 H, u32 mult, u32 ppad) noexcept {
	if (cond) { BVS->setViewportSizes(s32(W), s32(H), s32(mult), s32(ppad)); }
}

void SystemInterface::setDisplayBorderColor(u32 color) noexcept {
	BVS->setBorderColor(color);
}

f32 SystemInterface::getSystemFramerate() const noexcept {
	return mTargetFPS.load(mo::relaxed);
}

void SystemInterface::setSystemFramerate(f32 value) noexcept {
	mTargetFPS.store(value, mo::relaxed);
	Pacer->setLimiter(value);
}

void SystemInterface::saveOverlayData(const Str* data) {
	mOverlayData.store(std::make_shared<Str>(*data), mo::release);
}

Str* SystemInterface::makeOverlayData() {
	const auto frameMS{ Pacer->getElapsedMillisLast() };
	const auto elapsed{ Pacer->getElapsedMicrosSince() / 1000.0f };

	*getOverlayDataBuffer() = fmt::format(
		"Framerate:{:9.3f} fps |{:9.3f}ms\n"
		"Frametime:{:9.3f} ms ({:3.2f}%)\n",
		frameMS <= 0.0f ? getSystemFramerate()
			: std::round(1000.0f / frameMS * 100.0f) / 100.0f,
		frameMS, elapsed, elapsed / Pacer->getFramespan() * 100.0f
	);

	return getOverlayDataBuffer();
}

void SystemInterface::pushOverlayData() {
	if (Pacer->getValidFrameCounter() & 0x1) [[likely]] {
		saveOverlayData(SystemInterface::makeOverlayData());
	}
}

Str SystemInterface::copyOverlayData() const noexcept {
	return *mOverlayData.load(mo::acquire);
}
