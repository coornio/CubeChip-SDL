/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <iostream>
#include <sstream>
#include <fstream>

#include "BasicLogger.hpp"

/*==================================================================*/
	#pragma region BasicLogger Singleton Class
/*==================================================================*/

bool BasicLogger::initLogFile(
	const std::string&           filename,
	const std::filesystem::path& directory
) noexcept {
	if (filename.empty() || directory.empty()) {
		std::cerr << ":: ERROR :: " << "Log file name/path is invalid!" << std::endl;
		return true;
	}

	const auto newPath{ directory / filename };
	std::error_code error;

	if (std::filesystem::exists(newPath, error)) {
		if (!std::filesystem::remove_all(newPath, error) || error) {
			std::cerr << ":: ERROR :: " << "Unable to remove previous log file!" << std::endl;
			return true;
		}
	}

	mLogPath.assign(newPath);
	return false;
}

/*==================================================================*/

void BasicLogger::newEntry(const BLOG type, const std::string& message) noexcept {
	static std::size_t lineCount;

	std::ostringstream output;
	output << ++lineCount << " :: " << getSeverity(type) << " :: " << message;

	std::cout << output.str() << std::endl;

	if (!mLogPath.empty()) {
		std::ofstream logFile(mLogPath, std::ios::app);
		if (!logFile) {
			std::cerr << ":: ERROR :: " << "Unable to open log file: " << mLogPath << std::endl;
			mLogPath.clear();
			return;
		} else {
			logFile << output.str() << std::endl;
		}
	}
}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
