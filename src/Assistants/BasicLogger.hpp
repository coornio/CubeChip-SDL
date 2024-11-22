/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include <format>
#include <utility>
#include <filesystem>

#include "Typedefs.hpp"

/*==================================================================*/

enum class BLOG { INFO, WARN, ERROR, DEBUG };

/*==================================================================*/
	#pragma region BasicLogger Singleton Class

class BasicLogger final {
	BasicLogger() = default;
	BasicLogger(const BasicLogger&) = delete;
	BasicLogger& operator=(const BasicLogger&) = delete;

	Path mLogPath{};

	StrV getSeverity(BLOG type) const noexcept;

public:
	static auto* create() noexcept {
		static BasicLogger self;
		return &self;
	}

	bool initLogFile(const Str& filename, const Path& directory) noexcept;

private:
	void writeEntry(const BLOG type, const Str& message) noexcept;

public:
	template <typename... Args>
	void newEntry(const BLOG type, const Str& message, Args&&... args) noexcept {
		if constexpr (sizeof...(Args) == 0) {
			writeEntry(type, message);
		} else {
			writeEntry(type, std::vformat(message, std::make_format_args(args...)));
		}
	}
};

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

extern BasicLogger& blog;
