/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include <filesystem>

#include "../Assistants/BasicHome.hpp"

class HomeDirManager final : public BasicHome {
public:
	std::filesystem::path permRegs{};
	std::string path{};
	std::string file{};
	std::string name{};
	std::string type{};
	std::string sha1{};
	std::size_t hash{};
	std::size_t size{};

	HomeDirManager(const char*);

	void reset();
	void addDirectory();
	bool verifyFile(const char* = nullptr);
};
