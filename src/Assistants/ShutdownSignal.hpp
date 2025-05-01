/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <atomic>

/*==================================================================*/

class ShutdownSignal {
	static auto& requested_() noexcept {
		static std::atomic<bool> state{};
		return state;
	}

public:
	static void isRequested(bool state) noexcept { requested_().store(state, std::memory_order_release); }
	static bool isRequested()           noexcept { return requested_().load(std::memory_order_relaxed); }

	static void registerHandler() noexcept;
};
