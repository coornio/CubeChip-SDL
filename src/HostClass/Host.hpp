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

// needs to be on its own file
struct BasicAudioSpec final {
    const u32 outFrequency;
    SDL_AudioDeviceID device{};
    u32 volume{};
    s16 amplitude{};
    SDL_AudioSpec spec{};
    std::function<void(s16*, u32)> handler{};

    BasicAudioSpec();
    ~BasicAudioSpec();
    void setSpec(VM_Host*);
    void setVolume(s32);
    void pauseDevice(const bool);
    static void audioCallback(void*, u8*, s32);
};

class VM_Host final {
    bool _isReady{ false };
    bool _hasFile{ false };
public:
    bool benchmarking{ false };
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

    void runMachine(VM_Guest&);

    [[nodiscard]] bool isReady() const;
    [[nodiscard]] bool hasFile() const;
    void isReady(const bool);
    void hasFile(const bool);
};
