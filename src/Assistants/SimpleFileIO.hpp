/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <vector>
#include <fstream>
#include <utility>
#include <expected>
#include <filesystem>

#include "Typedefs.hpp"
#include "Concepts.hpp"

/*==================================================================*/

/* factory for std::expected type, <E> should be able to override as a boolean. */
template<typename T, typename E>
std::expected<T, E> make_expected(T&& value, E&& error) {
	if (error) { return std::unexpected(error); } else { return (value); }
}

namespace fs {
	/* Get last modification date of file at the designated path, if any. */
	[[maybe_unused]]
	inline auto last_write_time(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::last_write_time(filePath, error) };
		return make_expected(std::move(value), std::move(error));
	}

	/* Get size of file at the designated path, if any. */
	[[maybe_unused]]
	inline auto file_size(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::file_size(filePath, error) };
		return make_expected(std::move(value), std::move(error));
	}

	/*==================================================================*/

	/* Removes file or empty folder at the designated path, if any. */
	[[maybe_unused]]
	inline auto remove(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::remove(filePath, error) };
		return make_expected(std::move(value), std::move(error));
	}

	/* Removes all files/folders at the designated path, if any. */
	[[maybe_unused]]
	inline auto remove_all(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::remove_all(filePath, error) };
		return make_expected(std::move(value), std::move(error));
	}

	[[maybe_unused]]
	inline auto create_directory(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::create_directory(filePath, error) };
		return make_expected(std::move(value), std::move(error));
	}

	[[maybe_unused]]
	inline auto create_directory(const Path& filePath1, const Path& filePath2) noexcept {
		std::error_code error;
		auto value{ std::filesystem::create_directory(filePath1, filePath2, error) };
		return make_expected(std::move(value), std::move(error));
	}

	/* Create all required directories up to the designated path. */
	[[maybe_unused]]
	inline auto create_directories(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::create_directories(filePath, error) };
		return make_expected(std::move(value), std::move(error));
	}

	/* Check if the designated path leads to an existing location. */
	[[maybe_unused]]
	inline auto exists(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::exists(filePath, error) };
		return make_expected(std::move(value), std::move(error));
	}

	/* Check if the designated path leads to an existing, regular file. */
	[[maybe_unused]]
	inline auto is_regular_file(const Path& filePath) noexcept {
		std::error_code error;
		auto value{ std::filesystem::is_regular_file(filePath, error) };
		return make_expected(std::move(value), std::move(error));
	}
}

/*==================================================================*/

[[maybe_unused]]
inline auto readFileData(const Path& filePath) noexcept
-> std::expected<std::vector<char>, std::error_code>
{
	std::vector<char> fileData{};

	// get file's last write time stamp before we begin
	auto fileModStampBegin{ fs::last_write_time(filePath) };
	if (!fileModStampBegin) {
		return std::unexpected(std::move(fileModStampBegin.error()));
	}

	std::ifstream ifs(filePath, std::ios::binary);

	// check if the stream was unable to access the file
	if (!ifs) {
		return std::unexpected(std::make_error_code(std::errc::permission_denied));
	}

	// ensure we could copy all the data into the vector
	try {
		fileData.assign(std::istreambuf_iterator(ifs), {});
	} catch (const std::exception&) {
		return std::unexpected(std::make_error_code(std::errc::not_enough_memory));
	}

	// check if we failed in reading the file properly
	if (!ifs.good()) {
		return std::unexpected(std::make_error_code(std::errc::io_error));
	}

	// get file's last write time stamp after our read ended
	const auto fileModStampEnd{ fs::last_write_time(filePath) };
	if (!fileModStampEnd) {
		return std::unexpected(std::move(fileModStampEnd.error()));
	}

	// check if both timestamps are still the same
	if (fileModStampBegin.value() != fileModStampEnd.value()) {
		return std::unexpected(std::make_error_code(std::errc::interrupted));
	}

	return fileData;
}

/*==================================================================*/

template <typename T>
[[maybe_unused]]
inline auto writeFileData(const Path& filePath, const T* fileData, const usz fileSize) noexcept
-> std::expected<void, std::error_code>
{
	std::ofstream ofs(filePath, std::ios::binary);

	// check if the stream was unable to access the file
	if (!ofs) {
		return std::unexpected(std::make_error_code(std::errc::permission_denied));
	}

	// check if we failed in writing the file properly
	try {
		ofs.write(reinterpret_cast<const char*>(fileData), fileSize * sizeof(T));
		if (!ofs.good()) { throw std::exception{}; } else { return {}; }
	} catch (const std::exception&) {
		return std::unexpected(std::make_error_code(std::errc::io_error));
	}
}

template <IsContiguousContainer T>
[[maybe_unused]]
inline auto writeFileData(const Path& filePath, const T& fileData) noexcept {
	return writeFileData(filePath, std::data(fileData), std::size(fileData));
}

template <typename T, usz N>
[[maybe_unused]]
inline auto writeFileData(const Path& filePath, const T(&fileData)[N]) noexcept {
	return writeFileData(filePath, fileData, N);
}
