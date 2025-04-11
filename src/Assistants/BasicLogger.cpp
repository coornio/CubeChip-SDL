/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <utility>

#include "BasicLogger.hpp"
#include "SimpleFileIO.hpp"

/*==================================================================*/
	#pragma region BasicLogger Singleton Class

bool BasicLogger::initLogFile(const Str& filename, const Path& directory) noexcept {
	if (filename.empty() || directory.empty()) {
		newEntry(BLOG::ERROR, "Log file name/path cannot be blank!");
		return false;
	}

	const auto newPath{ directory / filename.c_str()};

	const auto fileExists{ fs::is_regular_file(newPath) };
	if (!fileExists) {
		newEntry(BLOG::ERROR,
			"Unable to ascertain if path to Log file is valid: \"{}\" [{}]",
			newPath.string(), fileExists.error().message());
		return false;
	}

	const auto fileDelete{ fs::remove_all(newPath) };
	if (!fileDelete) {
		newEntry(BLOG::ERROR,
			"Unable to remove previous Log file: \"{}\" [{}]",
			newPath.string(), fileDelete.error().message());
		return false;
	}

	mLogPath.assign(newPath);
	return true;
}

/*==================================================================*/

StrV BasicLogger::getSeverity(BLOG type) const noexcept {
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

void BasicLogger::writeEntry(BLOG type, const Str& message) {
	auto output{ fmt::format("{} :: {}", getSeverity(type), message) };

	if (mLogPath.empty()) { fmt::println("{}", std::move(output)); }
	else {
		std::ofstream logFile(mLogPath, std::ios::app);
		if (logFile) { logFile << std::move(output) << std::endl; }
		else {
			newEntry(BLOG::ERROR, "Unable to write to Log file: \"{}\"",
				std::move(mLogPath).string());
			fmt::println("{}", std::move(output));
		}
	}
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
