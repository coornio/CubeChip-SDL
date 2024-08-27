/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include <filesystem>

enum class BLOG { INFO, WARN, ERROR, DEBUG };

/*==================================================================*/
	#pragma region BasicLogger Singleton Class
/*==================================================================*/

class BasicLogger final {
	BasicLogger() = default;
	BasicLogger(const BasicLogger&) = delete;
	BasicLogger& operator=(const BasicLogger&) = delete;

	std::filesystem::path mLogPath{};

	std::string_view getSeverity(BLOG type) const noexcept {
		switch (type) {
			case BLOG::INFO:
				return "INFO";

			case BLOG::WARN:
				return "WARN";

			case BLOG::ERROR:
				return "ERROR";

			case BLOG::DEBUG:
				return "DEBUG";

			default:
				return "OTHER";
		}
	}

public:
	static BasicLogger& create() noexcept {
		static BasicLogger _self;
		return _self;
	}

	bool initLogFile(const std::string&, const std::filesystem::path&) noexcept;

	void newEntry(const BLOG type, const std::string&) noexcept;
};

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/

namespace blogger { // basic logger class
	extern BasicLogger& blog;
}
