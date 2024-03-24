/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../Includes.hpp"

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

class VM_Host final {
public:
    bool machineLoaded{ false };
    bool programLoaded{ false };
    bool benchmarking{ false };
    [[maybe_unused]] u64 cycles{};

    BasicRenderer& Render;
    HomeDirManager& File;

    explicit VM_Host(HomeDirManager&, BasicRenderer&, const char*);

    struct AudioSettings final {
        const u32 outFrequency;
        SDL_AudioDeviceID device{};
        u32 volume{};
        s16 amplitude{};
        SDL_AudioSpec spec{};
        std::function<void(s16*, u32)> handler{};

        AudioSettings();
        ~AudioSettings();
        void setSpec(VM_Host*);
        void setVolume(s32);
        static void audioCallback(void*, u8*, s32);
    } Audio;

    void runMachine(VM_Guest&);
    //void addMessage(std::string_view);

    [[nodiscard]] bool isReady() const;
    [[nodiscard]] bool romLoaded() const;
};
