/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <vector>
#include <fstream>
#include <utility>

#include "Typedefs.hpp"
#include "Concepts.hpp"

/*==================================================================*/

namespace fs {
	/* Get last modification date of file at the designated path, if any. */
	[[maybe_unused]]
	inline auto last_write_time(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::last_write_time(filePath, error) };
		return makeExpected(std::move(value), std::move(error));
	}

	/* Get size of file at the designated path, if any. */
	[[maybe_unused]]
	inline auto file_size(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::file_size(filePath, error) };
		return makeExpected(std::move(value), std::move(error));
	}

	/*==================================================================*/

	/* Removes file or empty folder at the designated path, if any. */
	[[maybe_unused]]
	inline auto remove(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::remove(filePath, error) };
		return makeExpected(std::move(value), std::move(error));
	}

	/* Removes all files/folders at the designated path, if any. */
	[[maybe_unused]]
	inline auto remove_all(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::remove_all(filePath, error) };
		return makeExpected(std::move(value), std::move(error));
	}

	[[maybe_unused]]
	inline auto create_directory(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::create_directory(filePath, error) };
		return makeExpected(std::move(value), std::move(error));
	}

	[[maybe_unused]]
	inline auto create_directory(const Path& filePath1, const Path& filePath2) noexcept {
		std::error_code error;
		auto value{ std::filesystem::create_directory(filePath1, filePath2, error) };
		return makeExpected(std::move(value), std::move(error));
	}

	/* Create all required directories up to the designated path. */
	[[maybe_unused]]
	inline auto create_directories(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::create_directories(filePath, error) };
		return makeExpected(std::move(value), std::move(error));
	}

	/* Check if the designated path leads to an existing location. */
	[[maybe_unused]]
	inline auto exists(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::exists(filePath, error) };
		return makeExpected(std::move(value), std::move(error));
	}

	/* Check if the designated path leads to an existing, regular file. */
	[[maybe_unused]]
	inline auto is_regular_file(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::is_regular_file(filePath, error) };
		return makeExpected(std::move(value), std::move(error));
	}
}

/*==================================================================*/

[[maybe_unused]]
inline auto readFileData(
	const Path& filePath, const usz dataReadSize = 0,
	const std::streamoff dataReadOffset = 0
) noexcept -> std::expected<std::vector<char>, std::error_code> {
	try {
		std::vector<char> fileData{};

		auto fileModStampBegin{ fs::last_write_time(filePath) };
		if (!fileModStampBegin) { return std::unexpected(std::move(fileModStampBegin.error())); }

		std::ifstream inFile(filePath, std::ios::binary);
		if (!inFile) { return std::unexpected(std::make_error_code(std::errc::permission_denied)); }

		inFile.seekg(static_cast<std::streampos>(dataReadOffset));
		if (!inFile) { return std::unexpected(std::make_error_code(std::errc::invalid_argument)); }
		
		if (dataReadSize) {
			fileData.resize(dataReadSize);
			inFile.read(fileData.data(), dataReadSize);
		} else {
			try {
				fileData.assign(std::istreambuf_iterator<char>(inFile), {});
			} catch (const std::exception&) {
				return std::unexpected(std::make_error_code(std::errc::not_enough_memory));
			}
		}

		if (!inFile.good()) { throw std::exception{}; }

		auto fileModStampEnd{ fs::last_write_time(filePath) };
		if (!fileModStampEnd) { return std::unexpected(std::move(fileModStampEnd.error())); }

		if (fileModStampBegin.value() != fileModStampEnd.value()) {
			return std::unexpected(std::make_error_code(std::errc::interrupted));
		} else { return fileData; }
	}
	catch (const std::exception&) {
		return std::unexpected(std::make_error_code(std::errc::io_error));
	}
}

/*==================================================================*/

template <typename T>
[[maybe_unused]]
inline auto writeFileData(
	const Path& filePath, const T* fileData, const usz dataWriteSize,
	const std::streamoff dataWriteOffset = 0
) noexcept -> std::expected<void, std::error_code> {
	try {
		std::ofstream outFile(filePath, std::ios::binary | std::ios::out);
		if (!outFile) { return std::unexpected(std::make_error_code(std::errc::permission_denied)); }

		outFile.seekp(static_cast<std::streampos>(dataWriteOffset));
		if (!outFile) { return std::unexpected(std::make_error_code(std::errc::invalid_argument)); }

		outFile.write(reinterpret_cast<const char*>(fileData), dataWriteSize * sizeof(T));
		if (!outFile.good()) { throw std::exception{}; } else { return {}; }
	}
	catch (const std::exception&) {
		return std::unexpected(std::make_error_code(std::errc::io_error));
	}
}

template <IsContiguousContainer T>
[[maybe_unused]]
inline auto writeFileData(
	const Path& filePath, const T& fileData, const usz dataWriteSize = 0,
	const std::streamoff dataWriteOffset = 0
) noexcept {
	return writeFileData(
		filePath, std::data(fileData), dataWriteSize
		? dataWriteSize : std::size(fileData),
		dataWriteOffset
	);
}

template <typename T, usz N>
[[maybe_unused]]
inline auto writeFileData(
	const Path& filePath, const T(&fileData)[N],
	const std::streamoff dataWriteOffset = 0
) noexcept {
	return writeFileData(filePath, fileData, N, dataWriteOffset);
}
