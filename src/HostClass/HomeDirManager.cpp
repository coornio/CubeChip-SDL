/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Host.hpp"
#include "../Assistants/PathExceptionClass.hpp"

/*------------------------------------------------------------------*/
/*  class  VM_Host::HomeDirManager                                  */
/*------------------------------------------------------------------*/

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
    size = 0;
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

    path = fspath.string();
    name = fspath.stem().string();
    type = fspath.extension().string();
    sha1 = SHA1::from_file(path);
    size = fslen;

    blog.stdLogOut("New file received: " + path);
    return true;
}
