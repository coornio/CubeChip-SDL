/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#pragma warning(push)
#pragma warning(disable : 26819) // C fallthrough warning disabled
#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_events.h>
#include <SDL_video.h>
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

using usz = std::size_t;
using u64 = std::uint64_t;
using s64 = std::int64_t;
using u32 = std::uint32_t;
using s32 = std::int32_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;
using u8  = std::uint8_t;
using s8  = std::int8_t;

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
using namespace std::string_view_literals;

class VM_Host;
class VM_Guest;

#include "Assistants/BasicLogger.hpp"
#include "Assistants/BasicRenderer.hpp"
#include "Assistants/BasicInput.hpp"

#include "Assistants/FrameLimiter.hpp"
#include "Assistants/HexInput.hpp"
#include "Assistants/Well512.hpp"
#include "Assistants/SHA1.hpp"

#include "_nlohmann/json.hpp"

// for convenience
using json = nlohmann::json;

using namespace blogger;
using namespace bic;
