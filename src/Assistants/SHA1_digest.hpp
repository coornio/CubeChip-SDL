// SPDX-FileCopyrightText: 2019-2022 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: (GPL-3.0 OR CC-BY-NC-ND-4.0)

#pragma once

#include <string>
#include <bit>

#include "../Includes.hpp"

class SHA1Digest {
    std::array<u32, 5> state{};
    std::array<u32, 2> count{};
    std::array<u8, 64> buffer{};
public:
    enum : u32 { DIGEST_SIZE = 20 };
    
    SHA1Digest();
    
    void update(const void* data, u32 len);
    void final(u8 digest[DIGEST_SIZE]);
    void reset();
    
    static std::string DigestToString(const u8 digest[DIGEST_SIZE]);
};
