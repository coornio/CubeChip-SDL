/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <iostream>
#include <sstream>
#include <fstream>

#include "BasicLogger.hpp"
#include "PathExceptionClass.hpp"

BasicLogger& blogger::blog{ BasicLogger::create() };

/*==================================================================*/
	#pragma region BasicLogger Singleton Class
/*==================================================================*/

void BasicLogger::createDirectory(
	const std::string&           filename,
	const std::filesystem::path& directory
) {
	if (filename.empty() || directory.empty()) {
		throw PathException("The log file must have a path/name!", "");
	}

	namespace fs = std::filesystem;

	fs::create_directories(directory);
	if (!fs::exists(directory) || !fs::is_directory(directory)) {
		throw PathException("Unable to create directory at: ", directory);
	}

	// append file.ext to directory, save to path
	mLogPath = directory / filename;

	// attempt to delete file if it exists already
	if (fs::exists(mLogPath)) {
		if (!fs::remove_all(mLogPath)) {
			throw PathException("Unable to clear old log file: ", mLogPath);
		}
	}
}

void BasicLogger::setStdLogFile(
	const std::string&           name,
	const std::filesystem::path& path
) {
	try {
		createDirectory(name, path);
	} catch (const std::exception& e) {
		std::cerr << ":: ERROR :: " << e.what() << std::endl;
		throw;
	}
}

/*==================================================================*/

void BasicLogger::newEntry(const BLOG type, const std::string& message) noexcept {
	static std::size_t lineCount;

	std::ostringstream output;
	output << ++lineCount << " :: " << getSeverity(type) << " :: " << message;

	std::cout << output.str() << std::endl;

	std::ofstream logFile(mLogPath, std::ios::app);
	if (!logFile) {
		std::cerr << "Unable to open log file: " << mLogPath << std::endl;
		return;
	} else {
		logFile << output.str() << std::endl;
	}
}

/*ΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛΛ*/
	#pragma endregion
/*VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV*/
