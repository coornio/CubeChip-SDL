#include <cstddef>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <iostream>

#ifndef MAP_NAMESPACE
    #define MAP_NAMESPACE

    #if defined(_WIN64) || defined(__x86_64__)
        using int_fast_t = std::int_fast64_t;
    #else
        using int_fast_t = std::int_fast32_t;
    #endif
#endif

template<typename T>
class MapRowProxy final {
    T* const         mBegin;
    const int_fast_t mLength;

public:
    explicit MapRowProxy(
        T* const         begin,
        const int_fast_t length
    ) noexcept
        : mBegin(begin)
        , mLength(length)
    {}

    T* begin()       noexcept { return mBegin; }
    T* begin() const noexcept { return mBegin; }
    T* end()         noexcept { return mBegin + mLength; }
    T* end()   const noexcept { return mBegin + mLength; }

    void wipeall() {
        std::fill(begin(), end(), T());
    }

    MapRowProxy<T>& wipe(
        const int_fast_t cols
    ) {
        if (std::abs(cols) >= mLength) {
            wipeall();
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

    MapRowProxy<T>& rotate(
        const int_fast_t cols
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

    MapRowProxy<T>& shift(
        const int_fast_t cols
    ) {
        if (std::abs(cols) < mLength) {
            rotate(cols);
        }
        wipe(cols);
        return *this;
    }

    T& at(
        const int_fast_t col
    ) {
        if (std::abs(col) >= mLength) {
            throw std::out_of_range("column index out of range");
        }
        return *(begin() + col);
    }

    T& operator[](
        const int_fast_t col
    ) {
        return *(begin() + col);
    }
};

template<typename T>
class Map2D {
    int_fast_t           mRows;
    int_fast_t           mCols;
    std::size_t          mSize;
    std::unique_ptr<T[]> pData;

    T* mBegin()       noexcept { return &pData.get()[0]; }
    T* mBegin() const noexcept { return &pData.get()[0]; }
    T* mEnd()         noexcept { return &pData.get()[mSize]; }
    T* mEnd()   const noexcept { return &pData.get()[mSize]; }

    void dimensionsCheck(
        const int_fast_t rows,
        const int_fast_t cols
    ) {
        if (rows <= 0 || cols <= 0) {
            throw std::out_of_range("map dimensions cannot be zero");
        }
        if (rows * cols > static_cast<std::size_t>(-1)) {
            throw std::out_of_range("map area exceeds integer limits");
        }
    }

public:
    Map2D(
        const int_fast_t rows,
        const int_fast_t cols
    )
        : mRows(rows)
        , mCols(cols)
        , mSize(rows* cols)
      #if (defined(_MSVC_LANG) && _MSVC_LANG >= 201402L) || __cplusplus >= 201402L
        , pData(std::make_unique<T[]>(mSize))
      #else 
        , pData(std::unique_ptr<T[]>(new T[mSize]))
      #endif
    {
        dimensionsCheck(rows, cols);
    }

    std::size_t size() const { return mSize; }
    int_fast_t  lenX() const { return mCols; }
    int_fast_t  lenY() const { return mRows; }

    Map2D<T>& resize(
        const int_fast_t rows,
        const int_fast_t cols
    ) {
        dimensionsCheck(rows, cols);

        const auto minRows{ std::min(rows, mRows) };
        const auto minCols{ std::min(cols, mCols) };

        mSize = rows * cols;

      #if (defined(_MSVC_LANG) && _MSVC_LANG >= 201402L) || __cplusplus >= 201402L
        auto pCopy{ std::make_unique<T[]>(mSize) };
      #else
        auto pCopy{ std::unique_ptr<T[]>(new T[mSize]) };
      #endif

        for (auto row{ 0 }; row < minRows; ++row) {
            const auto srcIdx{ pData.get() + row * mCols };
            const auto dstIdx{ pCopy.get() + row * cols  };
            std::copy(srcIdx, srcIdx + minCols, dstIdx);
        }

        mRows = rows;
        mCols = cols;

        pData = nullptr;
        pData = std::move(pCopy);

        return *this;
    }

    void wipeall() {
        std::fill(mBegin(), mEnd(), T());
    }

    Map2D<T>& wipeY(
        const int_fast_t rows
    ) {
        if (std::abs(rows) >= mRows) {
            wipeall();
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
        const int_fast_t cols
    ) {
        if (std::abs(cols) >= mCols) {
            wipeall();
        }
        else if (cols != 0) {
            for (auto row : *this) {
                row.wipe(cols);
            }
        }
        return *this;
    }

    Map2D<T>& rotateY(
        const int_fast_t rows
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
        const int_fast_t cols
    ) {
        if (std::abs(cols) % mCols) {
            for (auto row : *this) {
                row.rotate(cols);
            }
        }
        return *this;
    }

    Map2D<T>& shiftY(
        const int_fast_t rows
    ) {
        if (std::abs(rows) < mRows) {
            rotateY(rows);
        }
        wipeY(rows);
        return *this;
    }

    Map2D<T>& shiftX(
        const int_fast_t cols
    ) {
        if (std::abs(cols) < mCols) {
            rotateX(cols);
        }
        wipeX(cols);
        return *this;
    }

    T& at(
        const int_fast_t row,
        const int_fast_t col
    ) {
        if (row >= mRows || col >= mCols) {
            throw std::out_of_range("row or column index out of range");
        }
        return pData.get()[row * mCols + col];
    }

    MapRowProxy<T> at(
        const int_fast_t row
    ) {
        if (row >= mRows) {
            throw std::out_of_range("row index out of range");
        }
        return MapRowProxy<T>(mBegin() + row * mCols, mCols);
    }

    T& operator()(
        const int_fast_t row,
        const int_fast_t col
    ) {
        return pData.get()[row * mCols + col];
    }

    MapRowProxy<T> operator[](
        const int_fast_t row
    ) {
        return MapRowProxy<T>(mBegin() + row * mCols, mCols);
    }

private:
    class MapIterProxy {
        const T*         mBegin;
        const int_fast_t mLength;

    public:
        MapIterProxy(
            const T*         begin,
            const int_fast_t length
        ) noexcept
            : mBegin(begin)
            , mLength(length)
        {}

        MapRowProxy<T>& operator*() noexcept {
            return reinterpret_cast<MapRowProxy<T>&>(*this);
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
    MapIterProxy begin()       noexcept { return MapIterProxy(mBegin(), mCols); }
    MapIterProxy begin() const noexcept { return MapIterProxy(mBegin(), mCols); }
    MapIterProxy end()         noexcept { return MapIterProxy(mEnd(), mCols); }
    MapIterProxy end()   const noexcept { return MapIterProxy(mEnd(), mCols); }

    MapIterProxy cbegin() const noexcept { return begin(); }
    MapIterProxy cend()   const noexcept { return end();   }
};
