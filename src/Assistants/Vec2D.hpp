/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace v2d {
    #if defined(_WIN64) || defined(__x86_64__)
        using int_fast_t = std::int_fast64_t;
    #else
        using int_fast_t = std::int_fast32_t;
    #endif
}

template<typename T>
class VecRowProxy final {
    typename std::vector<T>::iterator mBegin;
    const    v2d::int_fast_t          mLength;

public:
    explicit VecRowProxy(
        typename std::vector<T>::iterator begin,
        const    v2d::int_fast_t          length
    ) noexcept
        : mBegin(begin)
        , mLength(length)
    {}

    void wipeall() {
        std::fill(mBegin, mBegin + mLength, T());
    }

    VecRowProxy<T>& wipe(
        const v2d::int_fast_t cols
    ) {
        if (std::abs(cols) >= mLength) {
            wipeall();
        }
        else if (cols != 0) {
            const auto mEnd{ mBegin + mLength };

            if (cols < 0) {
                std::fill(mEnd - std::abs(cols), mEnd, T());
            }
            else {
                std::fill(mBegin, mBegin + std::abs(cols), T());
            }
        }
        return *this;
    }

    VecRowProxy<T>& rotate(
        const v2d::int_fast_t cols
    ) {
        const auto offset{ std::abs(cols) % mLength };
        if (offset) {
            const auto mEnd{ mBegin + mLength };
            const auto pos{ cols < 0
                ? mBegin + offset
                : mEnd   - offset
            };

            std::rotate(mBegin, pos, mEnd);
        }
        return *this;
    }

    VecRowProxy<T>& shift(
        const v2d::int_fast_t cols
    ) {
        if (std::abs(cols) < mLength) {
            rotate(cols);
        }
        wipe(cols);
        return *this;
    }

    T& at(
        const v2d::int_fast_t col
    ) {
        if (std::abs(col) >= mLength) {
            throw std::out_of_range("column index out of range");
        }
        return *(mBegin + col);
    }

    T& operator[](
        const v2d::int_fast_t col
    ) {
        return *(mBegin + col);
    }

    typename std::vector<T>::iterator begin() { return mBegin; }
    typename std::vector<T>::iterator end()   { return mBegin + mLength; }
};

template<typename T>
class Vec2D {
    v2d::int_fast_t mRows;
    v2d::int_fast_t mCols;
    std::vector<T>  mData;

    void dimensionsCheck(
        const v2d::int_fast_t rows,
        const v2d::int_fast_t cols
    ) {
        if (rows <= 0 || cols <= 0) {
            throw std::out_of_range("vector dimensions cannot be zero");
        }
        if (rows * cols > static_cast<std::size_t>(-1)) {
            throw std::out_of_range("vector area exceeds integer limits");
        }
    }

public:
    Vec2D(
        const v2d::int_fast_t rows,
        const v2d::int_fast_t cols
    )
        : mRows(rows)
        , mCols(cols)
        , mData(rows * cols)
    {
        dimensionsCheck(rows, cols);
    }

    std::size_t     size() const { return mData.size(); }
    v2d::int_fast_t lenX() const { return mCols; }
    v2d::int_fast_t lenY() const { return mRows; }

    Vec2D<T>& resize(
        const v2d::int_fast_t rows,
        const v2d::int_fast_t cols
    ) {
        dimensionsCheck(rows, cols);

        mRows = rows;
        mCols = cols;
        mData.resize(rows * cols);
        mData.shrink_to_fit();
        return *this;
    }

    void wipeall() {
        std::fill(mData.begin(), mData.end(), T());
    }

    Vec2D<T>& wipe(
        const v2d::int_fast_t rows
    ) {
        if (std::abs(rows) >= mRows) {
            wipeall();
        }
        else if (rows != 0) {
            const auto offset{ mRows * std::abs(rows) };

            if (rows < 0) {
                std::fill(mData.end() - offset, mData.end(), T());
            }
            else {
                std::fill(mData.begin(), mData.begin() + offset, T());
            }
        }
        return *this;
    }

    Vec2D<T>& rotate(
        const v2d::int_fast_t rows
    ) {
        const auto offset{ mRows * (std::abs(rows) % mRows) };
        if (offset) {
            const auto pos{ rows < 0
                ? mData.begin() + offset
                : mData.end()   - offset
            };
            std::rotate(mData.begin(), pos, mData.end());
        }
        return *this;
    }

    Vec2D<T>& shift(
        const v2d::int_fast_t rows
    ) {
        if (std::abs(rows) < mRows) {
            rotate(rows);
        }
        wipe(rows);
        return *this;
    }

    T& at(
        const v2d::int_fast_t row,
        const v2d::int_fast_t col
    ) {
        if (row >= mRows || col >= mCols) {
            throw std::out_of_range("row or column index out of range");
        }
        return mData[row * mCols + col];
    }

    T& operator()(
        const v2d::int_fast_t row,
        const v2d::int_fast_t col
        ) {
        return mData[row * mCols + col];
    }

    VecRowProxy<T> at(
        const v2d::int_fast_t row
    ) {
        if (row >= mRows) {
            throw std::out_of_range("row index out of range");
        }
        return VecRowProxy<T>(mData.begin() + row * mCols, mCols);
    }

    VecRowProxy<T> operator[](
        const v2d::int_fast_t row
    ) {
        return VecRowProxy<T>(mData.begin() + row * mCols, mCols);
    }

    class ProxyIterator {
        typename std::vector<T>::iterator mBegin;
        const    v2d::int_fast_t          mLength;

    public:
        ProxyIterator(
            typename std::vector<T>::iterator begin,
            const    v2d::int_fast_t          length
        ) noexcept
            : mBegin(begin)
            , mLength(length)
        {}

        VecRowProxy<T>& operator*() {
            return reinterpret_cast<VecRowProxy<T>&>(*this);
        }

        ProxyIterator& operator++() noexcept {
            mBegin += mLength;
            return *this;
        }

        bool operator!=(
            const ProxyIterator& other
        ) const noexcept {
            return mBegin != other.mBegin;
        }
    };

    ProxyIterator begin() { return ProxyIterator(mData.begin(), mCols); }
    ProxyIterator end()   { return ProxyIterator(mData.end(),   mCols); }
};
