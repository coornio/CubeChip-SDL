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
#include <stdexcept>
#include <limits>
#include <type_traits>

template<typename T>
class Map2D {
    using rpT = typename std::remove_pointer<T>::type;
    const bool           mView;
    std::int_fast32_t    mRows;
    std::int_fast32_t    mCols;
    std::int_fast32_t    mPosY;
    std::int_fast32_t    mPosX;
    std::size_t          mSize;
    std::unique_ptr<T[]> pData;

    T* mBegin() const noexcept { return pData.get(); }
    T* mEnd()   const noexcept { return pData.get() + mSize; }

    class MapRowProxy final {
        T* const                mBegin;
        const std::int_fast32_t mLength;

    public:
        ~MapRowProxy() = default;
        explicit MapRowProxy(
            T* const                begin,
            const std::int_fast32_t length
        ) noexcept
            : mBegin(begin)
            , mLength(length)
        {}

        MapRowProxy& operator*() noexcept {
            return *this;
        }

        MapRowProxy& operator->() noexcept {
            return *this;
        }

        MapRowProxy& operator++() noexcept {
            mBegin += mLength;
            return *this;
        }

        MapRowProxy operator++(int) {
            auto tmp{ *this };
            mBegin += mLength;
            return tmp;
        }

        bool operator==(const MapRowProxy& other) const noexcept {
            return mBegin == other.mBegin;
        }

        bool operator!=(const MapRowProxy& other) const noexcept {
            return mBegin != other.mBegin;
        }

        T* begin()       noexcept { return mBegin; }
        T* begin() const noexcept { return mBegin; }
        T* end()         noexcept { return mBegin + mLength; }
        T* end()   const noexcept { return mBegin + mLength; }

        const T* cbegin() const noexcept { return begin(); }
        const T* cend()   const noexcept { return end(); }

        MapRowProxy& operator=(const MapRowProxy& other) {
            if (this != &other && other.mLength == mLength) {
                std::copy(other.begin(), other.end(), begin());
            }
            return *this;
        }

        MapRowProxy& swap(MapRowProxy&& other) noexcept {
            if (this != &other && mLength == other.mLength) {
                std::swap_ranges(begin(), end(), other.begin());
            }
            return *this;
        }

        //template<typename U, std::enable_if_t<std::is_arithmetic<U>::value, U> = true>
        template<typename U, typename =
            decltype(std::declval<T&>() *= std::declval<const U&>())>
        MapRowProxy& operator*=(const U& value) {
            for (T& elem : *this) {
                elem = static_cast<T>(elem * value);
            }
            return *this;
        }

        MapRowProxy& wipeAll() {
            std::fill(begin(), end(), T());
            return *this;
        }

        MapRowProxy& wipe(
            const std::int_fast32_t cols
        ) {
            if (std::abs(cols) >= mLength) {
                wipeAll();
            }
            else if (cols != 0) {
                if (cols < 0) {
                    std::fill(end() - std::abs(cols), end(), T());
                } else {
                    std::fill(begin(), begin() + std::abs(cols), T());
                }
            }
            return *this;
        }

        MapRowProxy& rotate(
            const std::int_fast32_t cols
        ) {
            if (cols != 0) {
                if (cols < 0) {
                    std::rotate(begin(), begin() + std::abs(cols) % mLength, end());
                } else {
                    std::rotate(begin(), end() - std::abs(cols) % mLength, end());
                }
            }
            return *this;
        }

        MapRowProxy& shift(
            const std::int_fast32_t cols
        ) {
            if (std::abs(cols) < mLength) {
                rotate(cols);
            }
            wipe(cols);
            return *this;
        }

        MapRowProxy& rev() {
            std::reverse(begin(), end());
            return *this;
        }

        T& at(
            const std::int_fast32_t col
        ) {
            if (std::abs(col) >= mLength) {
                throw std::out_of_range("column index out of range");
            }
            return *(begin() + col);
        }

        const T& at(
            const std::int_fast32_t col
        ) const {
            if (std::abs(col) >= mLength) {
                throw std::out_of_range("column index out of range");
            }
            return *(begin() + col);
        }

        T& operator[](
            const std::int_fast32_t col
        ) {
            return *(begin() + col);
        }

        const T& operator[](
            const std::int_fast32_t col
        ) const {
            return *(begin() + col);
        }
    };

    explicit Map2D(
        const bool              view,
        const std::int_fast32_t rows,
        const std::int_fast32_t cols,
        const std::int_fast32_t posY,
        const std::int_fast32_t posX
    )
        : mView(view)
        , mRows(rows)
        , mCols(cols)
        , mPosY(posY)
        , mPosX(posX)
        , mSize(rows * cols)
        , pData(std::make_unique<T[]>(mSize))
    {}

public:
    ~Map2D() = default;
    Map2D(Map2D&&) = default;

    Map2D(
        const Map2D& other
    )
        : Map2D(
            false,
            other.mRows,
            other.mCols,
            other.mPosY,
            other.mPosX
        )
    {
        std::copy(other.mBegin(), other.mEnd(), mBegin());
    }

    Map2D(
        const std::int_fast32_t rows = 1,
        const std::int_fast32_t cols = 1
    )
        : Map2D(
            false,
            std::max<std::int_fast32_t>(1, rows),
            std::max<std::int_fast32_t>(1, cols),
            0, 0
        )
    {}

    explicit Map2D( // should be used for first view()
        const std::int_fast32_t rows,
        const std::int_fast32_t cols,
        const std::int_fast32_t posY,
        const std::int_fast32_t posX,
        const Map2D<rpT>* const base
    )
        : Map2D(true, rows, cols, posY, posX)
    {
        for (auto y{ 0 }; y < mRows; ++y) {
            for (auto x{ 0 }; x < mCols; ++x) {
                at_raw(y, x) = const_cast<T>(&base->at_raw(y + posY, x + posX));
            }
        }
    }

    explicit Map2D( // should be used for nested view()
        const std::int_fast32_t  rows,
        const std::int_fast32_t  cols,
        const std::int_fast32_t  posY,
        const std::int_fast32_t  posX,
        const Map2D<rpT*>* const base
    )
        : Map2D(true, rows, cols, posY, posX)
    {
        for (auto y{ 0 }; y < mRows; ++y) {
            for (auto x{ 0 }; x < mCols; ++x) {
                at_raw(y, x) = base->at_raw(y + posY, x + posX);
            }
        }
    }

    Map2D& operator=(
        const Map2D& other
    ) {
        if (this != &other && mSize == other.mSize) {
            std::copy(other.begin(), other.end(), begin());
        }
        return *this;
    }
    Map2D& operator=(Map2D&&) = default;

    std::size_t       size() const { return mSize; }
    std::int_fast32_t lenX() const { return mCols; }
    std::int_fast32_t lenY() const { return mRows; }

    T& at_raw(
        const std::int_fast32_t idx
    ) {
        return pData.get()[idx];
    }

    const T& at_raw(
        const std::int_fast32_t idx
    ) const {
        return pData.get()[idx];
    }

    T& at_raw(
        const std::int_fast32_t row,
        const std::int_fast32_t col
    ) {
        return pData.get()[row * mCols + col];
    }

    const T& at_raw(
        const std::int_fast32_t row,
        const std::int_fast32_t col
    ) const {
        return pData.get()[row * mCols + col];
    }

    // view() functions differ only by return type, so I gotta duplicate and enable conditionally?
    template<typename U = T>
    typename std::enable_if<!std::is_pointer<U>::value, Map2D<T*>>::type view(
        std::int_fast32_t rows,
        std::int_fast32_t cols,
        std::int_fast32_t posY = 0,
        std::int_fast32_t posX = 0
    ) const {
        return Map2D<T*>( // first view() mode, returns object with pointers to original data
            std::min(std::max<std::int_fast32_t>(1, rows), mRows),
            std::min(std::max<std::int_fast32_t>(1, cols), mCols),
            std::min(std::max<std::int_fast32_t>(0, posY), std::abs(rows - mRows)),
            std::min(std::max<std::int_fast32_t>(0, posX), std::abs(cols - mCols)),
            this
        );
    }

    template<typename U = T>
    typename std::enable_if<std::is_pointer<U>::value, Map2D<T>>::type view(
        std::int_fast32_t rows,
        std::int_fast32_t cols,
        std::int_fast32_t posY = 0,
        std::int_fast32_t posX = 0
    ) const {
        return Map2D<T>( // chained view() mode, returns object with copied pointers
            std::min(std::max<std::int_fast32_t>(1, rows), mRows),
            std::min(std::max<std::int_fast32_t>(1, cols), mCols),
            std::min(std::max<std::int_fast32_t>(0, posY), std::abs(rows - mRows)),
            std::min(std::max<std::int_fast32_t>(0, posX), std::abs(cols - mCols)),
            this
        );
    }

    Map2D& linearCopy(
        const Map2D& other
    ) {
        const auto len{ std::min(mSize, other.mSize) };
        std::copy_n(other.mBegin(), len, mBegin());
        return *this;
    }

    Map2D& resize(
        std::int_fast32_t rows = 1,
        std::int_fast32_t cols = 1
    ) {
        rows = std::max(std::int_fast32_t{ 1 }, rows);
        cols = std::max(std::int_fast32_t{ 1 }, cols);

        if (rows == mRows && cols == mCols) return *this;

        const auto minRows{ std::min(rows, mRows) };
        const auto minCols{ std::min(cols, mCols) };

        mSize = rows * cols;

        auto pCopy{ std::make_unique<T[]>(mSize) };

        for (auto row{ 0 }; row < minRows; ++row) {
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

    Map2D& wipeAll() {
        std::fill(mBegin(), mEnd(), T());
        return *this;
    }

    Map2D& wipe(
        const std::int_fast32_t rows,
        const std::int_fast32_t cols
    ) {
        if (std::abs(rows) >= mRows || std::abs(cols) >= mCols) {
            wipeAll();
        }
        else {
            if (rows != 0) {
                if (rows < 0) {
                    std::fill(mEnd() + rows * mCols, mEnd(), T());
                } else {
                    std::fill(mBegin(), mBegin() + rows * mCols, T());
                }
            }
            if (cols != 0) {
                for (auto row : *this) {
                    row.wipe(cols);
                }
            }
        }
        return *this;
    }

    Map2D& rotate(
        const std::int_fast32_t rows,
        const std::int_fast32_t cols
    ) {
        if (std::abs(rows) % mRows) {
            if (rows < 0) {
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
        const std::int_fast32_t rows,
        const std::int_fast32_t cols
    ) {
        if (std::abs(rows) < mRows && std::abs(cols) < mCols) {
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
        for (auto row{ 0 }; row < mRows / 2; ++row) {
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
        if (mRows > 1 && mCols > 1) {
            for (std::size_t a{ 1 }, b{ 1 }; a < mSize - 1; b = ++a) {
                do {
                    b = (b % mRows) * mCols + (b / mRows);
                } while (b < a);

                if (b != a) std::iter_swap(mBegin() + a, mBegin() + b);
            }
        }
        std::swap(mRows, mCols);
        return *this;
    }

    T& at(
        const std::int_fast32_t row,
        const std::int_fast32_t col
    ) {
        if (row >= mRows || col >= mCols) {
            throw std::out_of_range("row or column index out of range");
        }
        return at_raw(row, col);
    }

    const T& at(
        const std::int_fast32_t row,
        const std::int_fast32_t col
    ) const {
        if (row >= mRows || col >= mCols) {
            throw std::out_of_range("row or column index out of range");
        }
        return at_raw(row, col);
    }

    MapRowProxy at(
        const std::int_fast32_t row
    ) {
        if (row >= mRows) {
            throw std::out_of_range("row index out of range");
        }
        return MapRowProxy(mBegin() + row * mCols, mCols);
    }

    const MapRowProxy at(
        const std::int_fast32_t row
    ) const {
        if (row >= mRows) {
            throw std::out_of_range("row index out of range");
        }
        return MapRowProxy(mBegin() + row * mCols, mCols);
    }

    T& operator()(
        const std::int_fast32_t row,
        const std::int_fast32_t col
    ) {
        return at_raw(row, col);
    }

    const T& operator()(
        const std::int_fast32_t row,
        const std::int_fast32_t col
    ) const {
        return at_raw(row, col);
    }

    MapRowProxy operator[](
        const std::int_fast32_t row
    ) {
        return MapRowProxy(mBegin() + row * mCols, mCols);
    }

    const MapRowProxy operator[](
        const std::int_fast32_t row
    ) const {
        return MapRowProxy(mBegin() + row * mCols, mCols);
    }

public:
    MapRowProxy begin() const noexcept { return MapRowProxy(mBegin(), mCols); }
    MapRowProxy end()   const noexcept { return MapRowProxy(mEnd(), mCols); }

    const MapRowProxy cbegin() const noexcept { return begin(); }
    const MapRowProxy cend()   const noexcept { return end(); }
};
