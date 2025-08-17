/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Millis.hpp"

#include <chrono>
#include <thread>

/*==================================================================*/

static const auto sInitialApplicationTimestamp
	{ std::chrono::steady_clock::now() };

/*==================================================================*/

long long Millis::now() noexcept {
	return std::chrono::duration_cast<std::chrono::milliseconds>
		(std::chrono::steady_clock::now() - sInitialApplicationTimestamp).count();
}

long long Millis::since(long long past_millis) noexcept {
	return now() - past_millis;
}

void Millis::sleep(unsigned long long millis) noexcept {
	using namespace std::chrono;

	if (millis >= 2) {
		std::this_thread::sleep_for(milliseconds(millis - 2));
	} else {
		auto target{ steady_clock::now() + milliseconds(millis) };
		do { std::this_thread::yield(); }
		while (steady_clock::now() < target);
	}
}
