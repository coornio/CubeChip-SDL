/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

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
[[msvc::forceinline]]
#else
[[gnu::always_inline]]
#endif
constexpr R as(T&& t) {
    return static_cast<R>(std::forward<T>(t));
}
template <typename R, typename T>
#ifdef _MSC_VER
[[msvc::forceinline]]
#else
[[gnu::always_inline]]
#endif
constexpr R to(T&& t) {
    return reinterpret_cast<R>(std::forward<T>(t));
}

constexpr std::size_t cexprHash(const char* str, std::size_t v = 0) noexcept {
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
