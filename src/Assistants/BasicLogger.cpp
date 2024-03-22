/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BasicLogger.hpp"
#include <iostream>
#include <fstream>

using namespace std::string_literals;
using namespace std::string_view_literals;

/*------------------------------------------------------------------*/
/*  class  BasicLogger                                              */
/*------------------------------------------------------------------*/

BasicLogger& BasicLogger::create() {
    if (!_self) _self = std::make_unique<BasicLogger>();
    return *_self.get();
}

std::unique_ptr<BasicLogger> BasicLogger::_self = nullptr;
BasicLogger& blogger::blog{ BasicLogger::create() };

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/

void BasicLogger::createDirectory(
    const std::string& filename,
    std::filesystem::path& directory,
    std::filesystem::path& logFilePath
) const {
    if (filename.empty() || directory.empty())
        throw std::invalid_argument("The log file must have a path/name!");

    std::filesystem::create_directories(directory);

    if (!std::filesystem::exists(directory))
        throw std::runtime_error("Unable to create directory at: " + directory.string());

    // append file.ext to directory, save to path
    logFilePath = directory / filename;

    // attempt to delete file if it exists already
    if (std::filesystem::exists(logFilePath))
        std::filesystem::remove(logFilePath);
}

bool BasicLogger::setStdLogFile(
    const std::string& name,
    std::filesystem::path& path
) {
    try {
        createDirectory(name, path, stdLogPath);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

bool BasicLogger::setErrLogFile(
    const std::string& name,
    std::filesystem::path& path
) {
    try {
        createDirectory(name, path, errLogPath);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/

void BasicLogger::writeLogFile(
    const std::string& message,
    std::filesystem::path& logFilePath,
    std::size_t& counter
) const {
    if (std::filesystem::exists(logFilePath)) {
        if (!std::filesystem::is_regular_file(logFilePath)) {
            throw std::runtime_error("Log file is malformed: " + logFilePath.string());
        }
    }

    std::ofstream logFile(logFilePath, std::ios::app);
    if (!logFile.is_open()) {
        throw std::runtime_error("Unable to access log file: " + logFilePath.string());
    }
    logFile << ++counter << " :: " << message << std::endl;
}

void BasicLogger::stdLogOut(const std::string& text) {
    try {
        writeLogFile(text, stdLogPath, cStd);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

void BasicLogger::errLogOut(const std::string& text) {
    try {
        writeLogFile(text, errLogPath, cErr);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
