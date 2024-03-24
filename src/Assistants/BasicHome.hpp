/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <filesystem>

class BasicHome {
    std::filesystem::path homeDir{};
protected:
    static bool showErrorBox(std::string_view, std::string_view);
    const std::filesystem::path& getHome() const noexcept { return homeDir; }

    BasicHome(const char*);
};
