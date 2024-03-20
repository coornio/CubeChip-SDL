/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Host.hpp"

/*------------------------------------------------------------------*/
/*  class  VM_Host::FileInfo                                         */
/*------------------------------------------------------------------*/

VM_Host::FileInfo::FileInfo(VM_Host& ref)
    : Host(ref)
{};

void VM_Host::FileInfo::reset() {
    Host.programLoaded = false;
    path = name = type = {};
    size = 0;
}

bool VM_Host::FileInfo::verifyFile(const char* newPath) {
    if (!newPath) return false;
    namespace fs = std::filesystem;

    const fs::path fspath{ newPath };
    if (!fs::exists(fspath) || !fs::is_regular_file(fspath)) {
        Host.addMessage("Unable to use locate path: "s + fspath.string(), false);
        return false;
    }

    std::error_code ec;
    const auto ifs_length{ fs::file_size(fspath, ec) };
    if (ec) {
        Host.addMessage("Unable to access file: "s + fspath.string(), false);
        return false;
    }
    if (ifs_length < 1) {
        Host.addMessage("File is empty: "s + fspath.string(), false);
        return false;
    }

    path = newPath;
    name = fspath.stem().string();
    type = fspath.extension().string();
    sha1 = SHA1::from_file(newPath);
    size = ifs_length;
    return true;
}
