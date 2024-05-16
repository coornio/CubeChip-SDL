/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <vector>
#include <stdexcept>
#include <utility>
#include <limits>
#include <type_traits>

template<class T>
concept arithmetic = std::is_arithmetic_v<T>;

template<class T>
concept integral = std::is_integral_v<T>;

template<class T>
concept ar_pointer = std::is_pointer_v<T> && std::is_arithmetic_v<std::remove_pointer_t<T>>;

template<typename T> requires arithmetic<T> || ar_pointer<T>
class MapRow : public std::vector<T> {
public:
	using std::vector<T>::vector;

	#pragma region MapRow+
	constexpr MapRow& operator+(const MapRow& other) {
		printf("ROW& + ROW& (A)\n");
		const auto mSize{ std::min(this->size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, mSize); ++i) {
			(*this)[i] += other[i];
		}
		return *this;
	}

	constexpr MapRow& operator+(MapRow&& other) {
		printf("ROW& + ROW&& (B)\n");
		const auto mSize{ std::min(this->size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, mSize); ++i) {
			(*this)[i] += std::move(other[i]);
		}
		return *this;
	}

	constexpr MapRow& operator+(const arithmetic auto& value) & {
		printf("ROW& + V& (C)\n");
		for (T& elem : *this) {
			elem += value;
		}
		return *this;
	}

	constexpr MapRow& operator+(const arithmetic auto& value) && {
		printf("ROW&& + VAL& (D)\n");
		for (T& elem : *this) {
			elem += value;
		}
		return *this;
	}

	#pragma endregion
	#pragma region Others
	#pragma endregion
};


template<typename T> requires arithmetic<T> || ar_pointer<T>
class Map2D {
	using paramS = std::int_fast32_t;
	using paramU = std::size_t;
	using underT = std::remove_pointer_t<T>;

	paramS     mRows;
	paramS     mCols;
	paramS     mPosY;
	paramS     mPosX;
	paramU     mSize;
	std::unique_ptr<T[]> pData;

	T* mBegin()  const noexcept { return pData.get(); }
	T* mEnd()    const noexcept { return pData.get() + mSize; }

	T* mBeginR() const noexcept { return mEnd() - 1; }
	T* mEndR()   const noexcept { return mBegin() - 1; }

	class RowProxy final {
		T*           mBegin;
		const paramS mLength;

	public:
		~RowProxy() = default;
		explicit RowProxy(
			T* const     begin,
			const paramS length
		) noexcept
			: mBegin(begin)
			, mLength(length) {}

		auto size() const { return mLength; }

		RowProxy& operator*() noexcept {
			return *this;
		}

		RowProxy* operator->() noexcept {
			return this;
		}

		RowProxy& operator++() noexcept {
			mBegin += mLength;
			return *this;
		}

		RowProxy& operator--() noexcept {
			mBegin -= mLength;
			return *this;
		}

		RowProxy operator++(int) noexcept {
			auto tmp{ *this };
			mBegin += mLength;
			return tmp;
		}

		RowProxy operator--(int) noexcept {
			auto tmp{ *this };
			mBegin -= mLength;
			return tmp;
		}

		bool operator==(const RowProxy& other) const noexcept {
			return mBegin == other.mBegin;
		}

		bool operator!=(const RowProxy& other) const noexcept {
			return mBegin != other.mBegin;
		}

		T* begin() const noexcept { return mBegin; }
		T* end()   const noexcept { return mBegin + mLength; }

		T* rbegin() const noexcept { return end() - 1; }
		T* rend()   const noexcept { return begin() - 1; }

		const T* cbegin() const noexcept { return begin(); }
		const T* cend()   const noexcept { return end(); }

		const T* crbegin() const noexcept { return rbegin(); }
		const T* crend()   const noexcept { return rend(); }

		MapRow<T> clone() const requires arithmetic<T> {
			MapRow<T> rowCopy(mBegin, mBegin + mLength);
			return rowCopy;
		}

		RowProxy& swap(
			RowProxy&& other
		) noexcept {
			if (this != &other && mLength == other.mLength) {
				std::swap_ranges(begin(), end(), other.begin());
			}
			return *this;
		}

		RowProxy& wipeAll() requires arithmetic<T> {
			std::fill(begin(), end(), T());
			return *this;
		}

		RowProxy& wipe(
			const integral auto cols
		) requires arithmetic<T> {
			if (std::cmp_greater_equal(std::abs(cols), mLength)) {
				wipeAll();
			} else if (std::cmp_not_equal(cols, 0)) {
				if (std::cmp_less(cols, 0)) {
					std::fill(end() - std::abs(cols), end(), T());
				} else {
					std::fill(begin(), begin() + std::abs(cols), T());
				}
			}
			return *this;
		}

		RowProxy& rotate(
			const integral auto cols
		) {
			if (std::cmp_not_equal(cols, 0)) {
				if (std::cmp_less(cols, 0)) {
					std::rotate(begin(), begin() + std::abs(cols) % mLength, end());
				} else {
					std::rotate(begin(), end() - std::abs(cols) % mLength, end());
				}
			}
			return *this;
		}

		RowProxy& shift(
			const integral auto cols
		) requires arithmetic<T> {
			if (std::cmp_less(std::abs(cols), mLength)) {
				rotate(std::forward<decltype(cols)>(cols));
			}
			wipe(std::forward<decltype(cols)>(cols));
			return *this;
		}

		RowProxy& rev() {
			std::reverse(begin(), end());
			return *this;
		}

	private:
		const paramS checkColBounds(
			const integral auto col
		) const {
			if (std::cmp_less(col, -mLength) || std::cmp_greater_equal(col, mLength)) {
				throw std::out_of_range("column index out of range");
			}
			if (std::cmp_less(col, 0)) return col + mLength;
			else return col;
		}

	public:
		/* bounds-checked accessors, reverse indexing allowed */

		auto at(const integral auto col)
		-> T&
		{ return *(begin() + checkColBounds(col)); }

		auto at(const integral auto col) const
		-> const T&
		{ return *(begin() + checkColBounds(col)); }

		/* unsafe accessors */
		auto operator[](const integral auto col)
		-> T&
		{ return *(begin() + col); }

		auto operator[](const integral auto col) const
		-> const T&
		{ return *(begin() + col); }





		#pragma region ROW =
		RowProxy & operator=(const arithmetic auto& value) {
			std::fill(begin(), end(), static_cast<T>(value));
			return *this;
		}
		RowProxy& operator=(const RowProxy& other) {
			printf("!!! RowProxy copy assignment of RowProxy\n");
			if (this == &other) [[unlikely]] return *this;
			const auto mSize{ std::min<paramU>(other.size(), mLength) };
			std::copy(other.begin(), other.begin() + mSize, mBegin);
			return *this;
		}
		RowProxy& operator=(MapRow<T>&& other) {
			printf("!!! RowProxy move assignment of MapRow\n");
			const auto mSize{ std::min<paramU>(other.size(), mLength) };
			std::move(other.begin(), other.begin() + mSize, mBegin);
			return *this;
		}
		RowProxy& operator=(const MapRow<T>& other) {
			printf("!!! RowProxy copy assignment of MapRow\n");
			if (this == &other) [[unlikely]] return *this;
			const auto mSize{ std::min<paramU>(other.size(), mLength) };
			std::copy(other.begin(), other.begin() + mSize, mBegin);
			return *this;
		}
		#pragma endregion

		#pragma region ROW +=
		RowProxy & operator+=(const arithmetic auto& value) {
			printf("RowProxy& += c_value& (1)\n");
			for (T& elem : *this) {
				elem += static_cast<T>(value);
			}
			return *this;
		}
		RowProxy& operator+=(const RowProxy& other) {
			printf("RowProxy& += RowProxy& (2)\n");
			const auto mSize{ std::min(other.mLength, mLength) };
			for (auto i{ 0 }; std::cmp_less(i, mSize); ++i) {
				(*this)[i] += other[i];
			}
			return *this;
		}
		RowProxy& operator+=(MapRow<T>&& other) {
			printf("RowProxy& += MapRow&& (3)\n");
			const auto mSize{ std::min<paramU>(other.size(), mLength) };
			for (auto i{ 0 }; std::cmp_less(i, mSize); ++i) {
				(*this)[i] += std::move(other[i]);
			}
			return *this;
		}
		RowProxy& operator+=(const MapRow<T>& other) {
			printf("RowProxy& += c_MapRow& (4)\n");
			const auto mSize{ std::min<paramU>(other.size(), mLength) };
			for (auto i{ 0 }; std::cmp_less(i, mSize); ++i) {
				(*this)[i] += other[i];
			}
			return *this;
		}
		#pragma endregion

	};

	explicit Map2D(
		const paramS rows,
		const paramS cols,
		const paramS posY,
		const paramS posX
	)
		: mRows(rows)
		, mCols(cols)
		, mPosY(posY)
		, mPosX(posX)
		, mSize(rows * cols)
		, pData(std::make_unique<T[]>(mSize))
	{}

public:
	~Map2D()       = default; // default destructor
	Map2D(Map2D&&) = default; // move constructor

	Map2D(const Map2D& other) // copy constructor
		: Map2D(
			other.mRows,
			other.mCols,
			other.mPosY,
			other.mPosX
		)
	{
		std::copy(other.mBegin(), other.mEnd(), mBegin());
	}

	Map2D() requires arithmetic<T>
		: Map2D(1, 1)
	{}

	Map2D(
		const integral auto rows,
		const integral auto cols
	) requires arithmetic<T>
		: Map2D(
			std::max<paramS>(1, std::abs(rows)),
			std::max<paramS>(1, std::abs(cols)),
			static_cast<paramS>(0),
			static_cast<paramS>(0)
		)
	{}

	explicit Map2D( // initial view() -> constructor :: type is T (T*), expects pointer to T (T)
		const paramS rows,
		const paramS cols,
		const paramS posY,
		const paramS posX,
		const Map2D<underT>* const base
	) requires ar_pointer<T>
		: Map2D(rows, cols, posY, posX)
	{
		for (auto y{ 0 }; std::cmp_less(y, mRows); ++y) {
			for (auto x{ 0 }; std::cmp_less(x, mCols); ++x) {
				at_raw(y, x) = const_cast<T>(&base->at_raw(y + posY, x + posX));
			}
		}
	}

	explicit Map2D( // chained view() -> constructor :: type is T (T*), expects pointer to T* (T)
		const paramS  rows,
		const paramS  cols,
		const paramS  posY,
		const paramS  posX,
		const Map2D* const base,
		const void* const
	) requires ar_pointer<T>
		: Map2D(rows, cols, posY, posX)
	{
		for (auto y{ 0 }; std::cmp_less(y, mRows); ++y) {
			for (auto x{ 0 }; std::cmp_less(x, mCols); ++x) {
				at_raw(y, x) = base->at_raw(y + posY, x + posX);
			}
		}
	}

	Map2D& operator=(Map2D&&) = default;   // move assignment
	Map2D& operator=(const Map2D& other) { // copy assignment
		if (this != &other && mSize == other.mSize) {
			std::copy(other.begin(), other.end(), begin());
		}
		return *this;
	}

	auto size() const { return mSize; }
	auto lenX() const { return mCols; }
	auto lenY() const { return mRows; }

	auto at_raw(const integral auto idx)
	-> T&
	{ return pData.get()[idx]; }

	auto at_raw(const integral auto idx) const
	-> const T&
	{ return pData.get()[idx]; }

	auto at_raw(const integral auto row, const integral auto col)
	-> T&
	{ return pData.get()[row * mCols + col]; }

	auto at_raw(const integral auto row, const integral auto col) const
	-> const T&
	{ return pData.get()[row * mCols + col]; }


	// initial view() requires T (T), returns as T*
	auto view(
		const paramS rows,
		const paramS cols,
		const paramS posY = 0,
		const paramS posX = 0
	) const requires arithmetic<T> {
		return Map2D<T*>(
			std::min(std::max<paramS>(1, rows), mRows),
			std::min(std::max<paramS>(1, cols), mCols),
			std::min(std::max<paramS>(0, posY), std::abs(rows - mRows)),
			std::min(std::max<paramS>(0, posX), std::abs(cols - mCols)),
			this
		);
	}

	// chained view() requires T (T*), returns as T*
	auto view(
		const paramS rows,
		const paramS cols,
		const paramS posY = 0,
		const paramS posX = 0
	) const requires ar_pointer<T> {
		return Map2D(
			std::min(std::max<paramS>(1, rows), mRows),
			std::min(std::max<paramS>(1, cols), mCols),
			std::min(std::max<paramS>(0, posY), std::abs(rows - mRows)),
			std::min(std::max<paramS>(0, posX), std::abs(cols - mCols)),
			this, nullptr
		);
	}


	Map2D& copyLinear(
		const Map2D& other
	) requires arithmetic<T> {
		const auto nSize{ std::min(mSize, other.mSize) };
		std::copy_n(other.mBegin(), nSize, mBegin());
		return *this;
	}

	Map2D& copyLinear(
		const T* const other,
		const integral auto size
	) requires arithmetic<T> {
		const auto nSize{ static_cast<paramU>(std::abs(size)) };
		std::copy_n(other, std::min(nSize, mSize), mBegin());
		return *this;
	}

	Map2D& resize(
		const bool choice,
		const integral auto rows,
		const integral auto cols
	) requires arithmetic<T> {
		const auto nRows{ std::max<paramS>(1, std::abs(rows)) };
		const auto nCols{ std::max<paramS>(1, std::abs(cols)) };

		enum COPY : bool { NO, YES };

		switch (choice) {
			case COPY::YES:
				if (nRows == mRows && nCols == mCols) return *this;
				return resizeCopy(nRows, nCols);

			case COPY::NO:
				return resizeWipe(nRows, nCols);
		}
		return *this;
	}

private:
	Map2D& resizeCopy(
		const paramS rows,
		const paramS cols
	) {
		const auto minRows{ std::min(rows, mRows) };
		const auto minCols{ std::min(cols, mCols) };

		mSize = rows * cols;

		auto pCopy{ std::make_unique<T[]>(mSize) };

		for (auto row{ 0 }; std::cmp_less(row, minRows); ++row) {
			const auto srcIdx{ pData.get() + row * mCols };
			const auto dstIdx{ pCopy.get() + row * cols };
			std::move(srcIdx, srcIdx + minCols, dstIdx);
		}

		mRows = rows;
		mCols = cols;

		pData = nullptr;
		pData = std::move(pCopy);

		return *this;
	}

	Map2D& resizeWipe(
		const paramS rows,
		const paramS cols
	) {
		mSize = rows * cols;
		mRows = rows;
		mCols = cols;

		pData = nullptr;
		pData = std::make_unique<T[]>(mSize);
		return *this;
	}

public:
	Map2D& wipeAll() requires arithmetic<T> {
		std::fill(mBegin(), mEnd(), T());
		return *this;
	}

	Map2D& wipe(
		const integral auto rows,
		const integral auto cols
	) requires arithmetic<T> {
		if (std::cmp_greater_equal(std::abs(rows), mRows) || std::cmp_greater_equal(std::abs(cols), mCols)) {
			wipeAll();
		} else {
			if (std::cmp_not_equal(rows, 0)) {
				if (std::cmp_less(rows, 0)) {
					std::fill(mEnd() + rows * mCols, mEnd(), T());
				} else {
					std::fill(mBegin(), mBegin() + rows * mCols, T());
				}
			}
			if (std::cmp_not_equal(cols, 0)) {
				for (auto& row : *this) {
					row.wipe(cols);
				}
			}
		}
		return *this;
	}

	Map2D& rotate(
		const integral auto rows,
		const integral auto cols
	) {
		if (std::abs(rows) % mRows) {
			if (std::cmp_less(rows, 0)) {
				std::rotate(mBegin(), mBegin() - rows * mCols, mEnd());
			} else {
				std::rotate(mBegin(), mEnd() - rows * mCols, mEnd());
			}
		}
		if (std::abs(cols) % mCols) {
			for (auto row : *this) {
				row.rotate(cols);
			}
		}
		return *this;
	}

	Map2D& shift(
		const integral auto rows,
		const integral auto cols
	) requires arithmetic<T> {
		if (std::cmp_less(std::abs(rows), mRows) && std::cmp_less(std::abs(cols), mCols)) {
			rotate(rows, cols);
		}
		wipe(rows, cols);
		return *this;
	}

	Map2D& rev() {
		std::reverse(mBegin(), mEnd());
		return *this;
	}

	Map2D& revY() {
		for (auto row{ 0 }; std::cmp_less(row, mRows / 2); ++row) {
			(*this)[row].swap((*this)[mRows - row - 1]);
		}
		return *this;
	}

	Map2D& revX() {
		for (auto row : *this) {
			std::reverse(row.begin(), row.end());
		}
		return *this;
	}

	Map2D& transpose() {
		if (std::cmp_greater(mRows, 1) || std::cmp_greater(mCols, 1)) {
			for (paramU a{ 1 }, b{ 1 }; std::cmp_less(a, mSize - 1); b = ++a) {
				do {
					b = (b % mRows) * mCols + (b / mRows);
				} while (std::cmp_less(b, a));

				if (b != a) std::iter_swap(mBegin() + a, mBegin() + b);
			}
		}
		std::swap(mRows, mCols);
		return *this;
	}

private:
	const paramS checkRowBounds(
		const integral auto row
	) const {
		if (std::cmp_less(row, -mRows) || std::cmp_greater_equal(row, mRows)) {
			throw std::out_of_range("row index out of range");
		}
		if (std::cmp_less(row, 0)) return row + mRows;
		else return row;
	}
	const paramS checkColBounds(
		const integral auto col
	) const {
		if (std::cmp_less(col, -mCols) || std::cmp_greater_equal(col, mCols)) {
			throw std::out_of_range("column index out of range");
		}
		if (std::cmp_less(col, 0)) return col + mCols;
		else return col;
	}

public:
	/* bounds-checked accessors, reverse indexing allowed */

	auto at(const integral auto row, const integral auto col)
	-> T&
	{ return at_raw(checkRowBounds(row), checkColBounds(col)); }

	auto at(const integral auto row, const integral auto col) const
	-> const T&
	{ return at_raw(checkRowBounds(row), checkColBounds(col)); }

	auto at(const integral auto row)
	-> RowProxy
	{ return RowProxy(mBegin() + checkRowBounds(row) * mCols, mCols); }

	auto at(const integral auto row) const
	-> const RowProxy
	{ return RowProxy(mBegin() + checkRowBounds(row) * mCols, mCols); }

/* unsafe accessors */

	auto operator() (const integral auto row, const integral auto col)
		-> T&
	{ return at_raw(row, col); }

	auto operator() (const integral auto row, const integral auto col) const
	-> const T&
	{ return at_raw(row, col); }

	auto operator[] (const integral auto row)
	-> RowProxy
	{ return RowProxy(mBegin() + row * mCols, mCols); }

	auto operator[] (const integral auto row) const
	-> const RowProxy
	{ return RowProxy(mBegin() + row * mCols, mCols); }

public:
	RowProxy begin() const noexcept { return RowProxy(mBegin(), mCols); }
	RowProxy end()   const noexcept { return RowProxy(mEnd(), mCols); }

	RowProxy rbegin() const noexcept { return RowProxy(mBeginR(), mCols); }
	RowProxy rend()   const noexcept { return RowProxy(mEndR(), mCols); }

	const RowProxy cbegin() const noexcept { return begin(); }
	const RowProxy cend()   const noexcept { return end(); }

	const RowProxy crbegin() const noexcept { return rbegin(); }
	const RowProxy crend()   const noexcept { return rend(); }
};
