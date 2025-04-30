/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <atomic>

/*==================================================================*/

class ShutdownSignal {
	static inline std::atomic<bool>
		sIsRequested{};

public:
	static void isRequested(bool state) noexcept { sIsRequested.store(state, std::memory_order_release); }
	static bool isRequested()           noexcept { return sIsRequested.load(std::memory_order_relaxed); }

	static void registerHandler() noexcept;
};
