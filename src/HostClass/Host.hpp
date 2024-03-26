/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../Includes.hpp"

// needs to be in its own file
class HomeDirManager final : public BasicHome {
public:
    std::filesystem::path permRegs{};
    std::string path{};
    std::string name{};
    std::string type{};
    std::string sha1{};
    std::size_t size{};

    HomeDirManager(const char*);

    void reset();
    void addDirectory();
    bool verifyFile(const char*);
};

// still want to manually init/exit the subsystem
// but where the hell will I put it???
struct BasicEventLoop {
    explicit BasicEventLoop() {
        SDL_InitSubSystem(SDL_INIT_EVENTS);
    }
    ~BasicEventLoop() {
        SDL_QuitSubSystem(SDL_INIT_EVENTS);
    }
};

//class BasicAudioSpec;
#include "BasicAudioSpec.hpp"

class VM_Host final {
    bool _isReady{ false };
    bool _hasFile{ false };
    bool _doBench{ false };
    s32  _state{};

public:
    [[maybe_unused]] u64 cycles{};

    HomeDirManager& File;
    BasicVideoSpec& Video;
    BasicAudioSpec& Audio;

    explicit VM_Host(
        HomeDirManager&,
        BasicVideoSpec&,
        BasicAudioSpec&,
        const char*
    );

    [[nodiscard]] bool isReady() const;
    [[nodiscard]] bool hasFile() const;
    [[nodiscard]] bool doBench() const;
    void isReady(const bool);
    void hasFile(const bool);
    void doBench(const bool);
};
