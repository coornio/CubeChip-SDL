/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <span>
#include <vector>
#include <fstream>
#include <concepts>
#include <filesystem>

#include "Typedefs.hpp"

/*==================================================================*/

[[maybe_unused]]
inline auto getFileModTime(
	const Path& filePath,
	std::error_code* const ioError = nullptr
) noexcept {
	std::error_code error;
	const auto modifiedTime{ std::filesystem::last_write_time(filePath, error) };
	if (error && ioError) { *ioError = error; }
	return modifiedTime;
}

[[maybe_unused]]
inline auto getFileSize(
	const Path& filePath,
	std::error_code* const ioError = nullptr
) noexcept {
	std::error_code error;
	const auto fileSize{ std::filesystem::file_size(filePath, error) };
	if (error && ioError) { *ioError = error; }
	return fileSize;
}

[[maybe_unused]]
inline auto doesPathExist(
	const Path& filePath,
	std::error_code* const ioError = nullptr
) noexcept {
	std::error_code error;

	// Check if the path doesn't lead anywhere
	if (!std::filesystem::exists(filePath, error) || error) {
		if (ioError) { *ioError = error; }
		return false;
	} else {
		return true;
	}
}

[[maybe_unused]]
inline auto doesFileExist(
	const Path& filePath,
	std::error_code* const ioError = nullptr
) noexcept {
	std::error_code error;

	// Check if the path doesn't lead to a regular file
	if (!std::filesystem::is_regular_file(filePath, error) || error) {
		if (ioError) { *ioError = error; }
		return false;
	} else {
		return true;
	}
}

[[maybe_unused]]
inline auto readFileData(
	const Path& filePath,
	std::error_code* const ioError = nullptr
) noexcept {
	std::vector<char> fileData{};
	std::error_code error;

	// Attempt first file mod time fetch
	const auto fileModStampBegin{ ::getFileModTime(filePath, &error) };
	if (error) {
		if (ioError) { *ioError = error; }
		fileData.clear(); return fileData;
	}

	std::ifstream ifs(filePath, std::ios::binary);

	// Attempt to init file stream
	if (ifs) {
		try {
			// Attempt to read data into vector
			fileData.assign(std::istreambuf_iterator(ifs), {});
		} catch (const std::exception&) {
			if (ioError) { *ioError = std::make_error_code(std::errc::not_enough_memory); }
			fileData.clear(); return fileData;
		}
		// Check if the stream failed to reach EOF safely
		if (!ifs.good()) {
			if (ioError) { *ioError = std::make_error_code(std::errc::io_error); }
			fileData.clear(); return fileData;
		}
	} else {
		if (ioError) {
			*ioError = std::make_error_code(std::errc::permission_denied);
			fileData.clear(); return fileData;
		}
	}

	// Attempt second file mod time fetch
	const auto fileModStampEnd{ ::getFileModTime(filePath, &error) };
	if (error) {
		if (ioError) { *ioError = error; }
		fileData.clear(); return fileData;
	}

	// Compare times to ensure no modification occurred during read
	if (fileModStampBegin != fileModStampEnd) {
		if (ioError) { *ioError = std::make_error_code(std::errc::interrupted); }
		fileData.clear(); return fileData;
	} else {
		return fileData;
	}
}

template <typename T> [[maybe_unused]]
inline auto writeFileData(
	const Path& filePath,
	std::span<const T> fileData,
	std::error_code* const ioError = nullptr
) noexcept requires std::is_trivially_constructible_v<T> {
	std::ofstream ofs(filePath, std::ios::binary);

	// Attempt to init file stream
	if (ofs) {
		ofs.write(reinterpret_cast<const char*>(fileData.data()), fileData.size() * sizeof(T));

		// Check if the stream failed to write fully
		if (!ofs.good()) {
			if (ioError) { *ioError = std::make_error_code(std::errc::io_error); }
			return true;
		} else {
			return false;
		}
	} else {
		if (ioError) { *ioError = std::make_error_code(std::errc::permission_denied); }
		return true;
	}
}
