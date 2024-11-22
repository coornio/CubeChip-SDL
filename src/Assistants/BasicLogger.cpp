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

bool BasicLogger::initLogFile(const Str8& filename, const Path& directory) noexcept {
	if (filename.empty() || directory.empty()) {
		std::cerr << ":: ERROR :: " << "Log file name/path is invalid!" << std::endl;
		return true;
	}

	const auto newPath{ directory / filename.c_str()};
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

void BasicLogger::writeEntry(const BLOG type, const Str& message) noexcept {
	static std::size_t lineCount;

#if _WIN32
	const auto wideMessage{ toStrW(message.c_str()) };

	std::wostringstream output;
	output << ++lineCount << L" :: " << getSeverity(type).data() << L" :: "
		<< wideMessage;

	std::wcout << output.str() << std::endl;

	if (!mLogPath.empty()) {
		std::wofstream logFile(mLogPath, std::ios::app);
		if (!logFile) {
			std::wcerr << L":: ERROR :: " << L"Unable to open log file: " << mLogPath << std::endl;
			mLogPath.clear();
			return;
		} else {
			logFile << output.str() << std::endl;
		}
	}
#else
	std::ostringstream output;
	output << ++lineCount << " :: " << getSeverity(type) << " :: "
		<< message.c_str();

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
#endif
}

	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
