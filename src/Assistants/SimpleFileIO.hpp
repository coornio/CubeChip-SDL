/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <span>
#include <vector>
#include <fstream>
#include <filesystem>

#include "Typedefs.hpp"
#include "Concepts.hpp"

/*==================================================================*/

[[maybe_unused]]
inline auto getFileModTime(const Path& filePath, std::error_code& ioError) noexcept {
	ioError.clear();
	return std::filesystem::last_write_time(filePath, ioError);
}

[[maybe_unused]]
inline auto getFileModTime(const Path& filePath) noexcept {
	std::error_code ioError;
	return getFileModTime(filePath, ioError);
}

[[maybe_unused]]
inline auto getFileSize(const Path& filePath, std::error_code& ioError) noexcept {
	ioError.clear();
	return std::filesystem::file_size(filePath, ioError);
}

[[maybe_unused]]
inline auto getFileSize(const Path& filePath) noexcept {
	std::error_code ioError;
	return getFileSize(filePath, ioError);
}

/*==================================================================*/

[[maybe_unused]]
inline bool doesPathExist(const Path& filePath, std::error_code& ioError) noexcept {
	ioError.clear();
	return std::filesystem::exists(filePath, ioError);
}

[[maybe_unused]]
inline auto doesPathExist(const Path& filePath) noexcept {
	std::error_code ioError;
	return doesPathExist(filePath, ioError);
}

[[maybe_unused]]
inline bool doesFileExist(const Path& filePath, std::error_code& ioError) noexcept {
	ioError.clear();
	return std::filesystem::is_regular_file(filePath, ioError);
}

[[maybe_unused]]
inline auto doesFileExist(const Path& filePath) noexcept {
	std::error_code ioError;
	return doesFileExist(filePath, ioError);
}

/*==================================================================*/

[[maybe_unused]]
inline auto readFileData(const Path& filePath, std::error_code& ioError) noexcept {
	using charVec = std::vector<char>;
	charVec fileData{};

	// Attempt first file mod time fetch
	const auto fileModStampBegin{ ::getFileModTime(filePath, ioError) };
	if (ioError) { return charVec{}; }

	std::ifstream ifs(filePath, std::ios::binary);

	// Attempt to init file stream
	if (ifs) {
		try {
			// Attempt to read data into vector
			fileData.assign(std::istreambuf_iterator(ifs), {});
		} catch (const std::exception&) {
			ioError = std::make_error_code(std::errc::not_enough_memory);
			return charVec{};
		}
		// Check if the stream failed to reach EOF safely
		if (!ifs.good()) {
			ioError = std::make_error_code(std::errc::io_error);
			return charVec{};
		}
	} else {
		ioError = std::make_error_code(std::errc::permission_denied);
		return charVec{};
	}

	// Attempt second file mod time fetch
	const auto fileModStampEnd{ ::getFileModTime(filePath, ioError) };
	if (ioError) { return charVec{}; }

	// Compare times to ensure no modification occurred during read
	if (fileModStampBegin != fileModStampEnd) {
		ioError = std::make_error_code(std::errc::interrupted);
		return charVec{};
	} else {
		return fileData;
	}
}

[[maybe_unused]]
inline auto readFileData(const Path& filePath) noexcept {
	std::error_code ioError;
	return readFileData(filePath, ioError);
}

/*==================================================================*/

template <typename T>
[[maybe_unused]]
inline bool writeFileData(const Path& filePath, const T* fileData, const usz fileSize, std::error_code& ioError) noexcept {
	std::ofstream ofs(filePath, std::ios::binary);

	if (!ofs) { // failed to open file stream
		ioError = std::make_error_code(std::errc::permission_denied);
		return false;
	}

	ofs.write(reinterpret_cast<const char*>(fileData), fileSize * sizeof(T));

	if (!ofs.good()) { // failed to write all data
		ioError = std::make_error_code(std::errc::io_error);
		return false;
	} else {
		return true;
	}
}

template <typename T>
[[maybe_unused]]
inline auto writeFileData(const Path& filePath, const T* fileData, const usz fileSize) noexcept {
	std::error_code ioError;
	return writeFileData(filePath, fileData, fileSize, ioError);
}

/*==================================================================*/

template <IsContiguousContainer T>
[[maybe_unused]]
inline bool writeFileData(const Path& filePath, const T& fileData, std::error_code& ioError) noexcept {
	return writeFileData(filePath, std::data(fileData), std::size(fileData), ioError);
}

template <IsContiguousContainer T>
[[maybe_unused]]
inline bool writeFileData(const Path& filePath, const T& fileData) noexcept {
	std::error_code ioError;
	return writeFileData(filePath, std::data(fileData), std::size(fileData), ioError);
}

template <typename T, usz N>
[[maybe_unused]]
inline bool writeFileData(const Path& filePath, const T(&fileData)[N], std::error_code& ioError) noexcept {
	return writeFileData(filePath, fileData, N, ioError);
}

template <typename T, usz N>
[[maybe_unused]]
inline bool writeFileData(const Path& filePath, const T(&fileData)[N]) noexcept {
	std::error_code ioError;
	return writeFileData(filePath, fileData, N, ioError);
}
