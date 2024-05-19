/*
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

/*
#include <utility>

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
*/

#include "Assistants/BasicHome.hpp"
#include "Assistants/BasicLogger.hpp"
#include "Assistants/BasicInput.hpp"
#include "Assistants/FrameLimiter.hpp"

#include "HostClass/HomeDirManager.hpp"
#include "HostClass/BasicVideoSpec.hpp"
#include "HostClass/BasicAudioSpec.hpp"

#include "HostClass/Host.hpp"
#include "GuestClass/Guest.hpp"

#pragma warning(push)
#pragma warning(disable : 26819) // C fallthrough warning disabled
#include "_nlohmann/json.hpp"
#pragma warning(pop)

// for convenience
using json = nlohmann::json;
using std::string_literals::operator""s;

using namespace blogger;
using namespace bic;
