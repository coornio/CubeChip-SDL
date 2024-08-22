/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstdint>
#include <string>
#include <filesystem>
#include <string_view>

#include "../Assistants/BasicHome.hpp"

class HomeDirManager final : public BasicHome {

	using GameValidator = bool (*)(std::uint64_t, std::string_view, std::string_view);

	GameValidator checkGame{};

public:
	std::filesystem::path permRegs{};
	std::string   path{};
	std::string   file{};
	std::string   name{};
	std::string   type{};
	std::string   sha1{};
	std::uint64_t size{};

	HomeDirManager(const std::string_view);

	void setValidator(GameValidator func) noexcept { checkGame = func; }

	void addDirectory();
	void clearCachedFileData() noexcept;
	bool validateGameFile(const char*) noexcept;
};
