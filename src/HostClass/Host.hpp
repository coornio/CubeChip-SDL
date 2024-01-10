#pragma once

#include "../Includes.hpp"

class VM_Host {
public:
    bool machineLoaded{ false };
    bool programLoaded{ false };
    bool benchmarking{ false };
    [[maybe_unused]] u64 cycles{};

    VM_Host(const char*);
    ~VM_Host();

    class FileInfo {
        VM_Host& Host;
    public:
        std::string path{};
        std::string name{};
        std::string type{};
        u64 size{};
        
        explicit FileInfo(VM_Host&);
        void reset();
        bool verifyFile(const char*);
    } File{ *this };

    struct AudioSettings {
        SDL_AudioDeviceID device{};
        const u32 outFrequency;
        SDL_AudioSpec spec{};
        float volume{};
        float volumeLog{};
        std::function<void(s16*, u32)> handler{};

        AudioSettings();
        void setSpec(VM_Host*);
        void setVolume(float);
        static void audioCallback(void*, u8*, s32);
    } Audio;

    struct RenderSettings {
        SDL_Window*   window{};
        SDL_Renderer* renderer{};
        SDL_Texture*  texture{};

        const std::string emuVersion{ "[8.12.23]"s };
              std::string title{};

        bool createWindow();
        bool createRenderer();

        void changeTitle(std::string_view);
        void errorMessage(const char*);

        //SDL_Rect screen{};
        float aspect{};

        s32 window_W{ 800 };
        s32 window_H{ 400 };

        s32  pitch{};
        u32* pixels{};

        void lockTexture();
        void unlockTexture();
        void setTexture(s16, s16, float);
        void setTextureAlpha(u8);
        void setTextureBlend(SDL_BlendMode);

        void present(bool);
    } Render;

    void runMachine(VM_Guest&);
    void addMessage(std::string_view, bool = true, u32 = 0);

    [[nodiscard]] bool machineValid() const;
    [[nodiscard]] bool programValid() const;
};
