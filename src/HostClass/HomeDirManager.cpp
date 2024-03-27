/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Host.hpp"
#include "../GuestClass/Guest.hpp"
#include "../Assistants/PathExceptionClass.hpp"

HomeDirManager::HomeDirManager(const char* homeName) try
    : BasicHome(homeName)
{
    try {
        blog.setStdLogFile("program.log", getHome());
        blog.setErrLogFile("debug.log", getHome());
        addDirectory();
    }
    catch (const std::exception& e) {
        BasicHome::showErrorBox(e.what(), "Fatal Initialization Error");
        throw;
    }
}
catch (const std::exception& e) {
    BasicHome::showErrorBox(e.what(), "Fatal Initilization Error");
    throw;
}

void HomeDirManager::reset() {
    path = name = type = sha1 = {};
    hash = size = 0;
}

void HomeDirManager::addDirectory() {
    permRegs = getHome() / "permRegs";
    std::filesystem::create_directories(permRegs);
    if (!std::filesystem::exists(permRegs)) {
        throw PathException("Could not create subdir: ", permRegs);
    }
}

bool HomeDirManager::verifyFile(const char* filepath) {
    if (!filepath) return false;
    namespace fs = std::filesystem;

    const fs::path fspath{ filepath };
    blog.stdLogOut("New file received: " + fspath.string());

    if (!fs::exists(fspath) || !fs::is_regular_file(fspath)) {
        blog.errLogOut("Unable to use locate path: " + fspath.string());
        return false;
    }

    std::error_code error;
    const auto fslen{ fs::file_size(fspath, error) };
    if (error) {
        blog.errLogOut("Unable to access file: " + fspath.string());
        return false;
    }
    if (fslen == 0) {
        blog.errLogOut("File is empty: " + fspath.string());
        return false;
    }

    auto tempPath{ fspath.string() };
    auto tempType{ fspath.extension().string() };
    auto tempHash{ cexprHash(tempType.c_str()) };
    auto tempSHA1{ SHA1::from_file(tempPath) };

    if (RomFileTypes::validate(tempHash, fslen, tempSHA1)) {
        path = tempPath;
        name = fspath.stem().string();
        type = tempType;
        sha1 = tempSHA1;
        hash = tempHash;
        size = fslen;

        return true;
    } else {
        return false;
    }
}
