/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	Adapted from public domain source code at:
		https://github.com/vog/sha1/blob/master/sha1.hpp
*/

#pragma once

#include <string>
#include <cstdint>
#include <filesystem>

class SHA1 {
	std::uint32_t digest[5]{};
	std::string   buffer{};
	std::uint64_t transforms{};

public:
	SHA1();
	void update(const std::string& s);
	void update(std::istream& is);
	std::string final();

	static std::string from_file(const std::filesystem::path& filePath);
};
