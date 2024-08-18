/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <iostream>
#include <fstream>

#include "BasicLogger.hpp"
#include "PathExceptionClass.hpp"

/*------------------------------------------------------------------*/
/*  class  BasicLogger                                              */
/*------------------------------------------------------------------*/

BasicLogger& BasicLogger::create() {
	return _self;
}

BasicLogger BasicLogger::_self = {};
BasicLogger& blogger::blog{ BasicLogger::create() };

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/

void BasicLogger::createDirectory(
	const std::string&           filename,
	const std::filesystem::path& directory,
	      std::filesystem::path& logFilePath
) const {
	if (filename.empty() || directory.empty())
		throw PathException("The log file must have a path/name!", "");

	std::filesystem::create_directories(directory);

	if (!std::filesystem::exists(directory))
		throw PathException("Unable to create directory at: ", directory);

	// append file.ext to directory, save to path
	logFilePath = directory / filename;

	// attempt to delete file if it exists already
	if (std::filesystem::exists(logFilePath)) {
		if (!std::filesystem::remove(logFilePath))
			throw PathException("Unable to clear old log file: ", logFilePath);
	}
}

void BasicLogger::setStdLogFile(
	const std::string&           name,
	const std::filesystem::path& path
) {
	try {
		createDirectory(name, path, stdLogPath);
	} catch (const std::exception& e) {
		std::cerr << ":: ERROR :: " << e.what() << std::endl;
		throw;
	}
}

void BasicLogger::setDbgLogFile(
	const std::string&           name,
	const std::filesystem::path& path
) {
	try {
		createDirectory(name, path, dbgLogPath);
	} catch (const std::exception& e) {
		std::cerr << ":: ERROR :: " << e.what() << std::endl;
		throw;
	}
}

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/

void BasicLogger::writeLogFile(
	const std::string&           message,
	const std::filesystem::path& logFilePath,
	      std::size_t&           counter
) const {
	if (std::filesystem::exists(logFilePath)) {
		if (!std::filesystem::is_regular_file(logFilePath)) {
			throw PathException("Log file is malformed: ", logFilePath);
		}
	}

	std::cout << ++counter << " :: " << message << std::endl;
	std::ofstream logFile(logFilePath, std::ios::app);
	if (!logFile.is_open()) {
		throw PathException("Unable to access log file: ", logFilePath);
	}
	logFile << counter << " :: " << message << std::endl;
}

void BasicLogger::stdLogOut(const std::string& text) {
	try {
		writeLogFile(text, stdLogPath, cStd);
	} catch (const std::exception& e) {
		std::cerr << ":: ERROR :: " << e.what() << std::endl;
	}
}

void BasicLogger::dbgLogOut(const std::string& text) {
	try {
		writeLogFile(text, dbgLogPath, cDbg);
	} catch (const std::exception& e) {
		std::cerr << ":: ERROR :: " << e.what() << std::endl;
	}
}
