/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.

	Adapted from public domain source code at:
		https://github.com/vog/sha1/blob/master/sha1.hpp
*/

#pragma once

#include <string>
#include <sstream>
#include <filesystem>

#include "Typedefs.hpp"
#include "Concepts.hpp"

class SHA1 {
	u32 digest[5]{};
	Str buffer{};
	u64 transforms{};

	void transform(u32* block);
	void buffer_to_block(u32* block);

public:
	SHA1() noexcept { reset(); }

	void reset() noexcept;

	void update(const Str& s);
	void update(std::istream& is);

	void update(const char* data, ust size);

	template <IsContiguousContainer T>
		requires (MatchingValueType<char, T>)
	void update(const T& data) noexcept {
		update(std::data(data), std::size(data));
	}

	Str final();

	static Str from_file(const Path& filePath);
	static Str from_data(const char* data, ust size);

	template <IsContiguousContainer T>
		requires (MatchingValueType<char, T>)
	static Str from_data(const T& data) {
		return from_data(std::data(data), std::size(data));
	}
};
