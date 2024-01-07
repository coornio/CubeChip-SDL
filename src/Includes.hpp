#pragma once

#pragma warning(push, 0)
#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_events.h>
#include <SDL_video.h>
#include <SDL_rect.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#pragma warning(pop)

#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string>
#include <vector>
#include <bitset>

#include <cstdint>
using u64 = uint64_t;
using s64 =  int64_t;
using u32 = uint32_t;
using s32 =  int32_t;
using u16 = uint16_t;
using s16 =  int16_t;
using u8  = uint8_t;
using s8  =  int8_t;

template <typename R, typename T>
#ifdef _MSC_VER
[[msvc::forceinline, msvc::flatten]]
#else
[[gnu::always_inline, gnu::flatten]]
#endif
constexpr R as(T&& t) noexcept {
    return static_cast<R>(std::forward<T>(t));
}
constexpr auto cexprHash(const char* str, std::size_t v = 0) noexcept -> std::size_t {
    return (*str == 0) ? v : 31 * cexprHash(str + 1) + *str;
}

template <typename T>
using vec2D = std::vector<std::vector<T>>;

template <typename T, auto X, auto Y = X>
using arr2D = std::array<std::array<T, X>, Y>;

using namespace std::string_literals;

class VM_Host;
class VM_Guest;

#include "Assistants/FrameLimiter.hpp"
#include "Assistants/BasicKeyInput.hpp"
#include "Assistants/HexInput.hpp"
#include "Assistants/Well512.hpp"
