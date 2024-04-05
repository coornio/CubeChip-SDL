/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cstddef>
#include <vector>
#include <algorithm>
#include <stdexcept>

template<typename T>
class VecRowProxy final {
    typename std::vector<T>::iterator mBegin;
    const    std::size_t              mLength;

public:
    explicit VecRowProxy(
        typename std::vector<T>::iterator begin,
        const    std::size_t              length
    ) noexcept
        : mBegin(begin)
        , mLength(length)
    {}

    void reset() {
        std::fill(mBegin, mBegin + mLength, T());
    }

    void reset(const long long cols) {
        if (cols == 0) return;
        if (std::abs(cols) >= mLength) {
            reset(); return;
        }

        const auto mEnd{ mBegin + mLength };

        if (cols < 0)
            std::fill(mEnd - std::abs(cols), mEnd, T());
        else
            std::fill(mBegin, mBegin + std::abs(cols), T());
    }

    void rotate(const long long cols) {
        const auto offset{ std::abs(cols) % mLength };

        if (!offset) return;

        const auto mEnd{ mBegin + mLength };
        const auto pos{ cols < 0
            ? mBegin + offset
            : mEnd - offset
        };

        std::rotate(mBegin, pos, mEnd);
    }

    T& at(const std::size_t col) {
        if (col >= mLength) {
            throw std::out_of_range("column index out of range");
        }
        return *(mBegin + col);
    }

    T& operator[](const std::size_t col) {
        return *(mBegin + col);
    }

    typename std::vector<T>::iterator begin() { return mBegin; }
    typename std::vector<T>::iterator end()   { return mBegin + mLength; }
};

template<typename T>
class Vec2D {
    std::size_t    mRows;
    std::size_t    mCols;
    std::vector<T> mData;

public:
    Vec2D(const std::size_t rows, const std::size_t cols)
        : mRows(rows)
        , mCols(cols)
        , mData(rows* cols)
    {
        if (!rows || !cols) {
            throw std::out_of_range("vector dimensions cannot be zero");
        }
    }

    void resize(const std::size_t rows, const std::size_t cols) {
        if (!rows || !cols) {
            throw std::out_of_range("vector dimensions cannot be zero");
        }
        mRows = rows;
        mCols = cols;
        mData.resize(rows * cols);
        mData.shrink_to_fit();
    }

    void reset() {
        std::fill(mData.begin(), mData.end(), T());
    }

    void reset(const long long rows) {
        if (rows == 0) return;
        if (std::abs(rows) >= mRows) {
            reset();
            return;
        }

        const auto offset{ mRows * std::abs(rows) };

        if (rows < 0)
            std::fill(mData.end() - offset, mData.end(), T());
        else
            std::fill(mData.begin(), mData.begin() + offset, T());
    }

    void rotate(const long long rows) {
        const auto offset{ mRows * (std::abs(rows) % mRows) };

        if (!offset) return;

        const auto pos{ rows < 0
            ? mData.begin() + offset
            : mData.end() - offset
        };

        std::rotate(mData.begin(), pos, mData.end());
    }

    std::size_t size() const { return mData.size(); }
    std::size_t lenX() const { return mCols; }
    std::size_t lenY() const { return mRows; }

    T& at(const std::size_t row, const std::size_t col) {
        if (row >= mRows || col >= mCols) {
            throw std::out_of_range("row or column index out of range");
        }
        return mData[row * mCols + col];
    }

    VecRowProxy<T> at(const std::size_t row) {
        if (row >= mRows) {
            throw std::out_of_range("row index out of range");
        }
        return VecRowProxy<T>(mData.begin() + row * mCols, mCols);
    }

    T& operator()(const std::size_t row, const std::size_t col) {
        return mData[row * mCols + col];
    }

    VecRowProxy<T> operator[](const std::size_t row) {
        return VecRowProxy<T>(mData.begin() + row * mCols, mCols);
    }

    class ProxyIterator {
        typename std::vector<T>::iterator mBegin;
        const    std::size_t              mLength;

    public:
        ProxyIterator(
            typename std::vector<T>::iterator begin,
            const    std::size_t              length
        ) noexcept
            : mBegin(begin)
            , mLength(length)
        {}

        VecRowProxy<T>& operator*() const { return *((VecRowProxy<T>*)(this)); }
        ProxyIterator& operator++() noexcept {
            mBegin += mLength;
            return *this;
        }
        bool operator!=(const ProxyIterator& other) const noexcept {
            return mBegin != other.mBegin;
        }
    };

    ProxyIterator begin() { return ProxyIterator(mData.begin(), mCols); }
    ProxyIterator end()   { return ProxyIterator(mData.end(), mCols); }
};
