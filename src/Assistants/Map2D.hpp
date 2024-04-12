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

#ifndef MAP_NAMESPACE
    #define MAP_NAMESPACE

    #if defined(_WIN64) || defined(__x86_64__)
        using fast_int = std::int_fast64_t;
    #else
        using fast_int = std::int_fast32_t;
    #endif
#endif


template<typename T>
class Map2D {
    fast_int             mRows;
    fast_int             mCols;
    std::size_t          mSize;
    std::unique_ptr<T[]> pData;

    T* mBegin()       noexcept { return pData.get(); }
    T* mBegin() const noexcept { return pData.get(); }
    T* mEnd()         noexcept { return pData.get() + mSize; }
    T* mEnd()   const noexcept { return pData.get() + mSize; }

    void dimensionsCheck(
        const fast_int rows,
        const fast_int cols
    ) {
        if (rows <= 0 || cols <= 0) {
            throw std::out_of_range("map dimensions cannot be zero");
        }
        if (static_cast<std::size_t>(rows * cols) > std::numeric_limits<std::size_t>::max()) {
            throw std::out_of_range("map area exceeds integer limits");
        }
    }

    class MapRowProxy final {
        T* const       mBegin;
        const fast_int mLength;

    public:
        ~MapRowProxy() = default;
        explicit MapRowProxy(
            T* const       begin,
            const fast_int length
        ) noexcept
            : mBegin(begin)
            , mLength(length)
        {}

        T* begin()       noexcept { return mBegin; }
        T* begin() const noexcept { return mBegin; }
        T* end()         noexcept { return mBegin + mLength; }
        T* end()   const noexcept { return mBegin + mLength; }

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

        MapRowProxy& wipeAll() {
            std::fill(begin(), end(), T());
            return *this;
        }

        MapRowProxy& wipe(
            const fast_int cols
        ) {
            if (std::abs(cols) >= mLength) {
                wipeAll();
            }
            else if (cols != 0) {
                if (cols < 0) {
                    std::fill(end() - std::abs(cols), end(), T());
                }
                else {
                    std::fill(begin(), begin() + std::abs(cols), T());
                }
            }
            return *this;
        }

        MapRowProxy& rotate(
            const fast_int cols
        ) {
            const auto offset{ std::abs(cols) % mLength };
            if (offset) {
                const auto pos{ cols < 0
                    ? begin() + offset
                    : end()   - offset
                };

                std::rotate(begin(), pos, end());
            }
            return *this;
        }

        MapRowProxy& shift(
            const fast_int cols
        ) {
            if (std::abs(cols) < mLength) {
                rotate(cols);
            }
            wipe(cols);
            return *this;
        }

        MapRowProxy& reverse() {
            std::reverse(begin(), end());
            return *this;
        }

        T& at(
            const fast_int col
        ) {
            if (std::abs(col) >= mLength) {
                throw std::out_of_range("column index out of range");
            }
            return *(begin() + col);
        }

        T& operator[](
            const fast_int col
        ) {
            return *(begin() + col);
        }
    };

    class MapIterProxy final {
        const T* mBegin;
        const fast_int mLength;

    public:
        MapIterProxy(
            const T* begin,
            const fast_int length
        ) noexcept
            : mBegin(begin)
            , mLength(length)
        {}

        MapRowProxy& operator*() noexcept {
            return reinterpret_cast<MapRowProxy&>(*this);
        }

        MapIterProxy& operator++() noexcept {
            mBegin += mLength;
            return *this;
        }

        bool operator!=(
            const MapIterProxy& other
        ) const noexcept {
            return mBegin != other.mBegin;
        }
    };

public:
    ~Map2D() = default;
    Map2D(
        const fast_int rows,
        const fast_int cols
    )
        : mRows(rows)
        , mCols(cols)
        , mSize(rows* cols)
        , pData(std::make_unique<T[]>(mSize))
    {
        dimensionsCheck(rows, cols);
    }

    std::size_t size() const { return mSize; }
    fast_int    lenX() const { return mCols; }
    fast_int    lenY() const { return mRows; }

    Map2D<T>& resize(
        const fast_int rows,
        const fast_int cols
    ) {
        dimensionsCheck(rows, cols);

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

    Map2D<T>& operator=(
        const Map2D<T>& other
    ) {
        if (this != &other && mSize == other.mSize) {
            std::copy(other.begin(), other.end(), begin());
        }
        return *this;
    }

    Map2D<T>& wipeAll() {
        std::fill(mBegin(), mEnd(), T());
        return *this;
    }

    Map2D<T>& wipeY(
        const fast_int rows
    ) {
        if (std::abs(rows) >= mRows) {
            wipeAll();
        }
        else if (rows != 0) {
            const auto offset{ mRows * std::abs(rows) };

            if (rows < 0) {
                std::fill(mEnd() - offset, mEnd(), T());
            }
            else {
                std::fill(mBegin(), mBegin() + offset, T());
            }
        }
        return *this;
    }

    Map2D<T>& wipeX(
        const fast_int cols
    ) {
        if (std::abs(cols) >= mCols) {
            wipeAll();
        }
        else if (cols != 0) {
            for (auto row : *this) {
                row.wipe(cols);
            }
        }
        return *this;
    }

    Map2D<T>& rotateYX(
        const fast_int rows,
        const fast_int cols
    ) {
        rotateY(rows);
        rotateX(cols);
        return *this;
    }

    Map2D<T>& rotateY(
        const fast_int rows
    ) {
        const auto offset{ mRows * (std::abs(rows) % mRows) };
        if (offset) {
            const auto pos{ rows < 0
                ? mBegin() + offset
                : mEnd()   - offset
            };
            std::rotate(mBegin(), pos, mEnd());
        }
        return *this;
    }

    Map2D<T>& rotateX(
        const fast_int cols
    ) {
        if (std::abs(cols) % mCols) {
            for (auto row : *this) {
                row.rotate(cols);
            }
        }
        return *this;
    }

    Map2D<T>& shiftYX(
        const fast_int rows,
        const fast_int cols
    ) {
        shiftY(rows);
        shiftX(cols);
        return *this;
    }

    Map2D<T>& shiftY(
        const fast_int rows
    ) {
        if (std::abs(rows) < mRows) {
            rotateY(rows);
        }
        wipeY(rows);
        return *this;
    }

    Map2D<T>& shiftX(
        const fast_int cols
    ) {
        if (std::abs(cols) < mCols) {
            rotateX(cols);
        }
        wipeX(cols);
        return *this;
    }

    Map2D<T>& reverseYX() {
        std::reverse(mBegin(), mEnd());
        return *this;
    }

    Map2D<T>& reverseY() {
        for (auto row{ 0 }; row < mRows / 2; ++row) {
            (*this)[row].swap((*this)[mRows - row - 1]);
        }
        return *this;
    }

    Map2D<T>& reverseX() {
        for (auto row : *this) {
            std::reverse(row.begin(), row.end());
        }
        return *this;
    }

    Map2D<T>& transpose() {
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
        const fast_int row,
        const fast_int col
    ) {
        if (row >= mRows || col >= mCols) {
            throw std::out_of_range("row or column index out of range");
        }
        return pData.get()[row * mCols + col];
    }

    MapRowProxy at(
        const fast_int row
    ) const {
        if (row >= mRows) {
            throw std::out_of_range("row index out of range");
        }
        return MapRowProxy(mBegin() + row * mCols, mCols);
    }

    T& operator()(
        const fast_int row,
        const fast_int col
    ) {
        return pData.get()[row * mCols + col];
    }

    MapRowProxy operator[](
        const fast_int row
    ) const {
        return MapRowProxy(mBegin() + row * mCols, mCols);
    }

private:


public:
    MapIterProxy begin()       noexcept { return MapIterProxy(mBegin(), mCols); }
    MapIterProxy begin() const noexcept { return MapIterProxy(mBegin(), mCols); }
    MapIterProxy end()         noexcept { return MapIterProxy(mEnd(), mCols); }
    MapIterProxy end()   const noexcept { return MapIterProxy(mEnd(), mCols); }

    MapIterProxy cbegin() const noexcept { return begin(); }
    MapIterProxy cend()   const noexcept { return end(); }
};
