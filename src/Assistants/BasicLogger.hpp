/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <memory>
#include <string>
#include <filesystem>

class BasicLogger final {
    static std::unique_ptr<BasicLogger> _self;

    BasicLogger() {};
    BasicLogger(const BasicLogger&) = delete;
    BasicLogger& operator=(const BasicLogger&) = delete;
    friend std::unique_ptr<BasicLogger> std::make_unique<BasicLogger>();

    std::filesystem::path stdLogPath{};
    std::filesystem::path errLogPath{};

    std::size_t cStd{}, cErr{};

    void createDirectory(
        const std::string&,
        const std::filesystem::path&,
        std::filesystem::path&
    ) const;
    void writeLogFile(
        const std::string&,
        const std::filesystem::path&,
        std::size_t&
    ) const;

public:
    static BasicLogger& create();

    void setStdLogFile(const std::string&, const std::filesystem::path&);
    void setErrLogFile(const std::string&, const std::filesystem::path&);

    void stdLogOut(const std::string&);
    void errLogOut(const std::string&);
};

namespace blogger { // basic logger class
    extern BasicLogger& blog;
}
