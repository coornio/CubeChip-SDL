/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <vector>
#include <stdexcept>
#include <utility>
#include <limits>
#include <type_traits>

#pragma region Concepts
template<class T>
concept integral = std::is_integral_v<T>;

template<class T>
concept arithmetic = std::is_arithmetic_v<T>;

template<class T>
concept ar_pointer = std::is_pointer_v<T> && std::is_arithmetic_v<std::remove_pointer_t<T>>;
#pragma endregion

#pragma region MapRow Class
template<typename T> requires arithmetic<T> || ar_pointer<T>
class MapRow : public std::vector<T> {
	using std::vector<T>::vector;

public:
	#pragma region operator +=
	constexpr MapRow& operator+=(
		const MapRow& other
	) requires arithmetic<T> {
		const auto len{ std::min(this->size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			(*this)[i] += other[i];
		}
		return *this;
	}
	constexpr MapRow& operator+=(
		const arithmetic auto& value
	) requires arithmetic<T> {
		for (T& elem : *this) {
			elem += value;
		}
		return *this;
	}
	#pragma endregion
	#pragma region operator -=
	constexpr MapRow& operator-=(
		const MapRow& other
	) requires arithmetic<T> {
		const auto len{ std::min(this->size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			(*this)[i] += other[i];
		}
		return *this;
	}
	constexpr MapRow& operator-=(
		const arithmetic auto& value
	) requires arithmetic<T> {
		for (T& elem : *this) {
			elem -= value;
		}
		return *this;
	}
	#pragma endregion
	#pragma region operator *=
	constexpr MapRow& operator*=(
		const MapRow& other
	) requires arithmetic<T> {
		const auto len{ std::min(this->size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			(*this)[i] *= other[i];
		}
		return *this;
	}
	constexpr MapRow& operator*=(
		const arithmetic auto& value
	) requires arithmetic<T> {
		for (T& elem : *this) {
			elem *= value;
		}
		return *this;
	}
	#pragma endregion
	#pragma region operator /=
	constexpr MapRow& operator/=(
		const MapRow& other
	) requires arithmetic<T> {
		const auto len{ std::min(this->size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			if (std::cmp_equal(other[i], 0)) {
				(*this)[i] = 0;
			} else {
				(*this)[i] /= other[i];
			}
		}
		return *this;
	}
	constexpr MapRow& operator/=(
		const arithmetic auto& value
	) requires arithmetic<T> {
		for (T& elem : *this) {
			if (std::cmp_equal(value, 0)) {
				elem = 0;
			} else {
				elem /= value;
			}
		}
		return *this;
	}
	#pragma endregion
	#pragma region operator %=
	constexpr MapRow& operator%=(
		const MapRow& other
	) requires arithmetic<T> {
		const auto len{ std::min(this->size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			if (std::cmp_equal(other[i], 0)) {
				(*this)[i] = 0;
			} else {
				(*this)[i] = std::fmod((*this)[i], other[i]);
			}
		}
		return *this;
	}
	constexpr MapRow& operator%=(
		const arithmetic auto& value
		) requires arithmetic<T> {
		for (T& elem : *this) {
			if (std::cmp_equal(value, 0)) {
				elem = 0;
			} else {
				elem = std::fmod(elem, value);
			}
		}
		return *this;
	}
	#pragma endregion

	#pragma region operator &=
	constexpr MapRow& operator&=(
		const MapRow& other
	) requires integral<T> {
		const auto len{ std::min(this->size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			(*this)[i] &= other[i];
		}
		return *this;
	}
	constexpr MapRow operator&=(
		const integral auto& value
	) requires integral<T> {
		for (T& elem : *this) {
			elem &= value;
		}
		return *this;
	}
	#pragma endregion
	#pragma region operator |=
	constexpr MapRow& operator|=(
		const MapRow& other
	) requires integral<T> {
		const auto len{ std::min(this->size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			(*this)[i] |= other[i];
		}
		return *this;
	}
	constexpr MapRow& operator|=(
		const integral auto& value
	) requires integral<T> {
		for (T& elem : *this) {
			elem |= value;
		}
		return *this;
	}
	#pragma endregion
	#pragma region operator ^=
	constexpr MapRow& operator^=(
		const MapRow& other
	) requires integral<T> {
		const auto len{ std::min(this->size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			(*this)[i] ^= other[i];
		}
		return *this;
	}
	constexpr MapRow& operator^=(
		const integral auto& value
	) requires integral<T> {
		for (T& elem : *this) {
			elem ^= value;
		}
		return *this;
	}
	#pragma endregion
	#pragma region operator <<=
	constexpr MapRow& operator<<=(
		const MapRow& other
	) requires integral<T> {
		const auto len{ std::min(this->size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			(*this)[i] <<= other[i];
		}
		return *this;
	}
	constexpr MapRow& operator<<=(
		const integral auto& value
	) requires integral<T> {
		for (T& elem : *this) {
			elem <<= value;
		}
		return *this;
	}
	#pragma endregion
	#pragma region operator >>=
	constexpr MapRow& operator>>=(
		const MapRow& other
	) requires integral<T> {
		const auto len{ std::min(this->size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			(*this)[i] >>= other[i];
		}
		return *this;
	}
	constexpr MapRow& operator>>=(
		const integral auto& value
	) requires integral<T> {
		for (T& elem : *this) {
			elem >>= value;
		}
		return *this;
	}
	#pragma endregion
	
	#pragma region operator +
	constexpr MapRow operator+(
		const MapRow& other
	) const requires arithmetic<T> {
		auto temp{ *this };
		const auto len{ std::min(temp.size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			temp[i] += other[i];
		}
		return temp;
	}
	constexpr MapRow operator+(
		const arithmetic auto& value
	) const requires arithmetic<T> {
		auto temp{ *this };
		for (T& elem : temp) {
			elem += value;
		}
		return temp;
	}
	#pragma endregion
	#pragma region operator -
	constexpr MapRow operator-(
		const MapRow& other
	) const requires arithmetic<T> {
		auto temp{ *this };
		const auto len{ std::min(temp.size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			temp[i] -= other[i];
		}
		return temp;
	}
	constexpr MapRow operator-(
		const arithmetic auto& value
	) const requires arithmetic<T> {
		auto temp{ *this };
		for (T& elem : temp) {
			elem -= value;
		}
		return temp;
	}
	#pragma endregion
	#pragma region operator *
	constexpr MapRow operator*(
		const MapRow& other
	) const requires arithmetic<T> {
		auto temp{ *this };
		const auto len{ std::min(temp.size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			temp[i] *= other[i];
		}
		return temp;
	}
	constexpr MapRow operator*(
		const arithmetic auto& value
	) const requires arithmetic<T> {
		auto temp{ *this };
		for (T& elem : temp) {
			elem *= value;
		}
		return temp;
	}
	#pragma endregion
	#pragma region operator /
	constexpr MapRow operator/(
		const MapRow& other
	) const requires arithmetic<T> {
		auto temp{ *this };
		const auto len{ std::min(temp.size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			if (std::cmp_equal(other[i], 0)) {
				temp[i] = 0;
			} else {
				temp[i] /= other[i];
			}
		}
		return temp;
	}
	constexpr MapRow operator/(
		const arithmetic auto& value
	) const requires arithmetic<T> {
		auto temp{ *this };
		for (T& elem : temp) {
			if (std::cmp_equal(value, 0)) {
				elem = 0;
			} else {
				elem /= value;
			}
		}
		return temp;
	}
	#pragma endregion
	#pragma region operator %
	constexpr MapRow operator%(
		const MapRow& other
	) const requires arithmetic<T> {
		auto temp{ *this };
		const auto len{ std::min(temp.size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			if (std::cmp_equal(other[i], 0)) {
				temp[i] = 0;
			} else {
				temp[i] /= std::fmod(temp[i], other[i]);
			}
		}
		return temp;
	}
	constexpr MapRow operator%(
		const arithmetic auto& value
	) const requires arithmetic<T> {
		auto temp{ *this };
		for (T& elem : temp) {
			if (std::cmp_equal(value, 0)) {
				elem = 0;
			} else {
				elem = std::fmod(elem, value);;
			}
		}
		return temp;
	}
	#pragma endregion
	
	#pragma region operator &
	constexpr MapRow operator&(
		const MapRow& other
	) const requires integral<T> {
		auto temp{ *this };
		const auto len{ std::min(temp.size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			temp[i] &= other[i];
		}
		return temp;
	}
	constexpr MapRow operator&(
		const integral auto& value
	) const requires integral<T> {
		auto temp{ *this };
		for (T& elem : temp) {
			elem &= value;
		}
		return temp;
	}
	#pragma endregion
	#pragma region operator |
	constexpr MapRow operator|(
		const MapRow& other
	) const requires integral<T> {
		auto temp{ *this };
		const auto len{ std::min(temp.size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			temp[i] |= other[i];
		}
		return temp;
	}
	constexpr MapRow operator|(
		const integral auto& value
	) const requires integral<T> {
		auto temp{ *this };
		for (T& elem : temp) {
			elem |= value;
		}
		return temp;
	}
	#pragma endregion
	#pragma region operator ^
	constexpr MapRow operator^(
		const MapRow& other
	) const requires integral<T> {
		auto temp{ *this };
		const auto len{ std::min(temp.size(), other.size()) };
		for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
			temp[i] ^= other[i];
		}
		return temp;
	}
	constexpr MapRow operator^(
		const integral auto& value
	) const requires integral<T> {
		auto temp{ *this };
		for (T& elem : temp) {
			elem ^= value;
		}
		return temp;
	}
	#pragma endregion
	#pragma region operator ~
	constexpr MapRow operator~() const & requires integral<T> {
		auto temp{ *this };
		for (T& elem : temp) {
			elem = ~elem;
		}
		return temp;
	}
	constexpr MapRow&& operator~() && requires integral<T> {
		for (T& elem : *this) {
			elem = ~elem;
		}
		return std::move(*this);
	}
	#pragma endregion
	#pragma region operator !
	constexpr MapRow operator!() const & requires integral<T> {
		auto temp{ *this };
		for (T& elem : temp) {
			elem = !elem;
		}
		return temp;
	}
	constexpr MapRow&& operator!() && requires integral<T> {
		for (T& elem : *this) {
			elem = !elem;
		}
		return std::move(*this);
	}
	#pragma endregion
};
#pragma endregion

#pragma region Map2D Class
template<typename T> requires arithmetic<T> || ar_pointer<T>
class Map2D final {
	using paramS = std::int_fast32_t;
	using paramU = std::size_t;
	using underT = std::remove_const_t<std::remove_pointer_t<T>>;

	paramS mRows;
	paramS mCols;
	paramU mSize;
	std::unique_ptr<T[]> pData;

public:
	auto size() const { return mSize; }
	auto lenX() const { return mCols; }
	auto lenY() const { return mRows; }

public:
	#pragma region Raw Accessors
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
	#pragma endregion

private:
	#pragma region RowProxy Class
	class RowProxy {
	protected:
		T*           mBegin;
		const paramS mLength;

	public:
		auto size() const { return mLength; }

	public:
		#pragma region Ctor
		explicit RowProxy(
			T* const     begin,
			const paramS length
		) noexcept
			: mBegin { begin  }
			, mLength{ length }
		{}
		#pragma endregion

	public:
		#pragma region clone() :: Matrix[R]
		/**
		 * @brief Clones the row's data and returns a vector of it.
		 * @return Vector of the same type.
		 */
		MapRow<T> clone() const requires arithmetic<T> {
			MapRow<T> rowCopy(mBegin, mBegin + mLength);
			return rowCopy;
		}
		#pragma endregion
		
		#pragma region swap() :: Matrix[R] + View[R]
		/**
		 * @brief Swaps the row's data with the data from another row.
		 * @return Self reference for method chaining.
		 */
		RowProxy& swap(
			RowProxy&& other
		) noexcept {
			if (this != &other && mLength == other.mLength) {
				std::swap_ranges(begin(), end(), other.begin());
			}
			return *this;
		}
		#pragma endregion
		
		#pragma region wipeAll() :: Matrix[R]
		/**
		 * @brief Wipes the row's data by default initializing it.
		 *
		 * @return Self reference for method chaining.
		 */
		RowProxy& wipeAll() requires arithmetic<T> {
			std::fill(begin(), end(), T());
			return *this;
		}
		#pragma endregion
		
		#pragma region wipe() :: Matrix[R]
		/**
		 * @brief Wipes the row's data in a given direction.
		 * @return Self reference for method chaining.
		 * 
		 * @param[in] cols :: Total items to wipe. Directional.
		 *
		 * @warning The sign of the param controls the application direction.
		 * @warning If the param exceeds row length, all row data is wiped.
		 */
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
		#pragma endregion
		
		#pragma region rotate() :: Matrix[R] + View[R]
		/**
		 * @brief Rotates the row's data in a given direction.
		 * @return Self reference for method chaining.
		 * 
		 * @param[in] cols :: Total positions to rotate. Directional.
		 *
		 * @warning The sign of the param controls the application direction.
		 */
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
		#pragma endregion
		
		#pragma region shift() :: Matrix[R]
		/**
		 * @brief Rotates the row's data in a given direction. Combines the
		 * functionality of rotating and wiping.
		 * @return Self reference for method chaining.
		 * 
		 * @param[in] cols :: Total positions to shift. Directional.
		 *
		 * @warning The sign of the param controls the application direction.
		 * @warning If the param exceeds row length, all row data is wiped.
		 */
		RowProxy& shift(
			const integral auto cols
		) requires arithmetic<T> {
			if (std::cmp_less(std::abs(cols), mLength)) {
				rotate(cols);
			}
			return wipe(cols);
		}
		#pragma endregion
		
		#pragma region shiftBit() :: Matrix[R]
		/**
		 * @brief Shifts the row's data in a bitwise manner in a given direction.
		 * @return Self reference for method chaining.
		 *
		 * @param[in] cols  :: Total column bit positions to shift. Directional.
		 * @param[in] limit :: Apply a bit limiter. Defaults to T size, optional.
		 *
		 * @warning The sign of the param controls the application direction.
		 * @warning If the param exceeds row length, all row data is wiped.
		 * @warning Row bit shifting is limited in distance to matrix T type.
		 */
		RowProxy& shiftBit(
			const integral auto cols,
			const paramU limit = sizeof(T) * 8
		) requires integral<T> {
			if (std::cmp_not_equal(cols, 0)) {
				const auto limiter{ (1 << limit) - 1 };

				if (std::cmp_less(cols, 0)) {
					for (auto X{ 0 }; X < mLength; ++X) {
						auto temp{ (*this)[X] << -cols };
						if (X < mLength - 1) {
							temp |= (*this)[X + 1] >> (limit + cols);
						}
						(*this)[X] = temp & limiter;
					}
				} else {
					for (auto X{ mLength - 1 }; X >= 0; --X) {
						auto temp{ (*this)[X] >> cols };
						if (X > 0) {
							temp |= (*this)[X - 1] << (limit - cols);
						}
						(*this)[X] = temp & limiter;
					}
				}
			}
			return *this;
		}
		#pragma endregion
		
		#pragma region shiftBitMask() :: Matrix[R]
		/**
		 * @brief Shifts the row's data in a marked bitwise manner in a given direction.
		 * @return Self reference for method chaining.
		 *
		 * @param[in] cols  :: Total column bit positions to shift. Directional.
		 * @param[in] mask  :: Bit mask to control the shift pattern.
		 * @param[in] limit :: Apply a bit limiter. Defaults to T size, optional.
		 *
		 * @warning The sign of the param controls the application direction.
		 * @warning If the param exceeds row length, all row data is wiped.
		 * @warning Row bit shifting is limited in distance to matrix T type.
		 */
		RowProxy& shiftBitMask(
			const integral auto cols,
			const integral auto mask,
			const paramU limit = sizeof(T) * 8
		) requires integral<T> {
			if (std::cmp_not_equal(cols, 0)) {
				const auto limiter{ (1 << limit) - 1 };

				if (std::cmp_less(cols, 0)) {
					for (auto X{ 0 }; X < mLength; ++X) {
						auto temp{ (*this)[X] << -cols };
						if (X < mLength - 1) {
							temp |= (*this)[X + 1] >> (limit + cols);
						}
						(*this)[X] &= ~mask;
						(*this)[X] |= temp & (mask & limiter);
					}
				} else {
					for (auto X{ mLength - 1 }; X >= 0; --X) {
						auto temp{ (*this)[X] >> cols };
						if (X > 0) {
							temp |= (*this)[X - 1] << (limit - cols);
						}
						(*this)[X] &= ~mask;
						(*this)[X] |= temp & (mask & limiter);
					}
				}
			}
			return *this;
		}
		#pragma endregion
		
		#pragma region reverse() :: Matrix[R] + View[R]
		/**
		 * @brief Reverses the row's data.
		 * @return Self reference for method chaining.
		 */
		RowProxy& reverse() {
			std::reverse(begin(), end());
			return *this;
		}
		#pragma endregion

	private:
		#pragma region Accessor Bounds Checker
		paramS checkColBounds(
			const integral auto col
		) const {
			if (std::cmp_less(col, -mLength) || std::cmp_greater_equal(col, mLength)) {
				throw std::out_of_range("column index out of range");
			}
			if (std::cmp_less(col, 0)) return col + mLength;
			else return col;
		}
		#pragma endregion

	public:
		#pragma region Accessors
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
		#pragma endregion

	public:
		#pragma region operator =
		RowProxy& operator=(
			const arithmetic auto& value
		) requires arithmetic<T> {
			std::fill(begin(), end(), static_cast<T>(value));
			return *this;
		}
		RowProxy& operator=(
			const RowProxy& other
		) requires arithmetic<T> {
			if (this == &other) [[unlikely]] return *this;
			const auto len{ std::min<paramU>(other.size(), mLength) };
			std::copy(other.begin(), other.begin() + len, mBegin);
			return *this;
		}
		RowProxy& operator=(
			MapRow<T>&& other
		) requires arithmetic<T> {
			const auto len{ std::min<paramU>(other.size(), mLength) };
			std::move(other.begin(), other.begin() + len, mBegin);
			return *this;
		}
		RowProxy& operator=(
			const MapRow<T>& other
		) requires arithmetic<T> {
			const auto len{ std::min<paramU>(other.size(), mLength) };
			std::copy(other.begin(), other.begin() + len, mBegin);
			return *this;
		}
		#pragma endregion
		
		#pragma region operator +=
		RowProxy& operator+=(
			const arithmetic auto& value
		) requires arithmetic<T> {
			for (T& elem : *this) {
				elem += value;
			}
			return *this;
		}
		RowProxy& operator+=(
			const RowProxy& other
		) requires arithmetic<T> {
			const auto len{ std::min(other.mLength, mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] += other[i];
			}
			return *this;
		}
		RowProxy& operator+=(
			const MapRow<T>& other
		) requires arithmetic<T> {
			const auto len{ std::min<paramU>(other.size(), mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] += other[i];
			}
			return *this;
		}
		#pragma endregion
		#pragma region operator -=
		RowProxy& operator-=(
			const arithmetic auto& value
		) requires arithmetic<T> {
			for (T& elem : *this) {
				elem -= value;
			}
			return *this;
		}
		RowProxy& operator-=(
			const RowProxy& other
		) requires arithmetic<T> {
			const auto len{ std::min(other.mLength, mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] -= other[i];
			}
			return *this;
		}
		RowProxy& operator-=(
			const MapRow<T>& other
		) requires arithmetic<T> {
			const auto len{ std::min<paramU>(other.size(), mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] -= other[i];
			}
			return *this;
		}
		#pragma endregion
		#pragma region operator *=
		RowProxy& operator*=(
			const arithmetic auto& value
		) requires arithmetic<T> {
			for (T& elem : *this) {
				elem *= value;
			}
			return *this;
		}
		RowProxy& operator*=(
			const RowProxy& other
		) requires arithmetic<T> {
			const auto len{ std::min(other.mLength, mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] *= other[i];
			}
			return *this;
		}
		RowProxy& operator*=(
			const MapRow<T>& other
		) requires arithmetic<T> {
			const auto len{ std::min<paramU>(other.size(), mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] *= other[i];
			}
			return *this;
		}
		#pragma endregion
		#pragma region operator /=
		RowProxy & operator/=(
			const arithmetic auto& value
		) requires arithmetic<T> {
			for (T& elem : *this) {
				if (std::cmp_equal(value, 0)) {
					elem = 0;
				} else {
					elem /= value;
				}
			}
			return *this;
		}
		RowProxy& operator/=(
			const RowProxy& other
		) requires arithmetic<T> {
			const auto len{ std::min(other.mLength, mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				if (std::cmp_equal(other[i], 0)) {
					(*this)[i] = 0;
				} else {
					(*this)[i] /= other[i];
				}
			}
			return *this;
		}
		RowProxy& operator/=(
			const MapRow<T>& other
		) requires arithmetic<T> {
			const auto len{ std::min<paramU>(other.size(), mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				if (std::cmp_equal(other[i], 0)) {
					(*this)[i] = 0;
				} else {
					(*this)[i] /= other[i];
				}
			}
			return *this;
		}
		#pragma endregion
		#pragma region operator %=
		RowProxy & operator%=(
			const arithmetic auto& value
		) requires arithmetic<T> {
			for (T& elem : *this) {
				if (std::cmp_equal(value, 0)) {
					elem = 0;
				} else {
					elem = std::fmod(elem, value);
				}
			}
			return *this;
		}
		RowProxy& operator%=(
			const RowProxy& other
		) requires arithmetic<T> {
			const auto len{ std::min(other.mLength, mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				if (std::cmp_equal(other[i], 0)) {
					(*this)[i] = 0;
				} else {
					(*this)[i] = std::fmod((*this)[i], other[i]);
				}
			}
			return *this;
		}
		RowProxy& operator%=(
			const MapRow<T>& other
		) requires arithmetic<T> {
			const auto len{ std::min<paramU>(other.size(), mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				if (std::cmp_equal(other[i], 0)) {
					(*this)[i] = 0;
				} else {
					(*this)[i] = std::fmod((*this)[i], other[i]);
				}
			}
			return *this;
		}
		#pragma endregion
		
		#pragma region operator &=
		RowProxy& operator&=(
			const integral auto& value
		) requires integral<T> {
			for (T& elem : *this) {
				elem &= value;
			}
			return *this;
		}
		RowProxy& operator&=(
			const RowProxy& other
		) requires integral<T> {
			const auto len{ std::min(other.mLength, mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] &= other[i];
			}
			return *this;
		}
		RowProxy& operator&=(
			const MapRow<T>& other
		) requires integral<T> {
			const auto len{ std::min<paramU>(other.size(), mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] &= other[i];
			}
			return *this;
		}
		#pragma endregion
		#pragma region operator |=
		RowProxy& operator|=(
			const integral auto& value
		) requires integral<T> {
			for (T& elem : *this) {
				elem |= value;
			}
			return *this;
		}
		RowProxy& operator|=(
			const RowProxy& other
		) requires integral<T> {
			const auto len{ std::min(other.mLength, mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] |= other[i];
			}
			return *this;
		}
		RowProxy& operator|=(
			const MapRow<T>& other
		) requires integral<T> {
			const auto len{ std::min<paramU>(other.size(), mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] |= other[i];
			}
			return *this;
		}
		#pragma endregion
		#pragma region operator ^=
		RowProxy& operator^=(
			const integral auto& value
		) requires integral<T> {
			for (T& elem : *this) {
				elem ^= value;
			}
			return *this;
		}
		RowProxy& operator^=(
			const RowProxy& other
		) requires integral<T> {
			const auto len{ std::min(other.mLength, mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] ^= other[i];
			}
			return *this;
		}
		RowProxy& operator^=(
			const MapRow<T>& other
		) requires integral<T> {
			const auto len{ std::min<paramU>(other.size(), mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] ^= other[i];
			}
			return *this;
		}
		#pragma endregion
		#pragma region operator <<=
		RowProxy& operator<<=(
			const integral auto& value
		) requires integral<T> {
			for (T& elem : *this) {
				elem <<= value;
			}
			return *this;
		}
		RowProxy& operator<<=(
			const RowProxy& other
		) requires integral<T> {
			const auto len{ std::min(other.mLength, mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] <<= other[i];
			}
			return *this;
		}
		RowProxy& operator<<=(
			const MapRow<T>& other
		) requires integral<T> {
			const auto len{ std::min<paramU>(other.size(), mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] <<= other[i];
			}
			return *this;
		}
		#pragma endregion
		#pragma region operator >>=
		RowProxy& operator>>=(
			const integral auto& value
		) requires integral<T> {
			for (T& elem : *this) {
				elem >>= value;
			}
			return *this;
		}
		RowProxy& operator>>=(
			const RowProxy& other
		) requires integral<T> {
			const auto len{ std::min(other.mLength, mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] >>= other[i];
			}
			return *this;
		}
		RowProxy& operator>>=(
			const MapRow<T>& other
		) requires integral<T> {
			const auto len{ std::min<paramU>(other.size(), mLength) };
			for (auto i{ 0 }; std::cmp_less(i, len); ++i) {
				(*this)[i] >>= other[i];
			}
			return *this;
		}
		#pragma endregion

	public:
		#pragma region Iterator begin/end
		T* begin() const noexcept { return mBegin; }
		T* end()   const noexcept { return mBegin + mLength; }

		T* rbegin() const noexcept { return end() - 1; }
		T* rend()   const noexcept { return begin() - 1; }

		const T* cbegin() const noexcept { return begin(); }
		const T* cend()   const noexcept { return end(); }

		const T* crbegin() const noexcept { return rbegin(); }
		const T* crend()   const noexcept { return rend(); }
		#pragma endregion
	};
	#pragma endregion

	#pragma region RowIterator Class
	class RowIterator final : private RowProxy {
		using diff_t = std::ptrdiff_t;
		
		T*&           _ptr;
		const paramS& _len;

	public:
		#pragma region Ctor
		explicit RowIterator(
			T* const     begin,
			const paramS length
		) noexcept
			: RowProxy{ begin, length }
			, _ptr    { this->mBegin  }
			, _len    { this->mLength }
		{}
		#pragma endregion

	public:
		#pragma region Iterator Overloads
		RowProxy& operator* () noexcept { return *this; }
		RowProxy* operator->() noexcept { return  this; }

		RowIterator& operator++() noexcept { _ptr += _len; return *this; }
		RowIterator& operator--() noexcept { _ptr -= _len; return *this; }

		RowIterator operator++(int) noexcept { auto tmp{ *this }; _ptr += _len; return tmp; }
		RowIterator operator--(int) noexcept { auto tmp{ *this }; _ptr -= _len; return tmp; }
		
		RowIterator  operator+ (const diff_t rhs) const { return RowIterator(_ptr + rhs * _len, _len); }
		RowIterator  operator- (const diff_t rhs) const { return RowIterator(_ptr - rhs * _len, _len); }
		
		RowIterator& operator+=(const diff_t rhs) { _ptr += rhs * _len; return *this; }
		RowIterator& operator-=(const diff_t rhs) { _ptr -= rhs * _len; return *this; }

		friend RowIterator operator+(const diff_t lhs, const RowIterator& rhs) { return rhs + lhs; }
		friend RowIterator operator-(const diff_t lhs, const RowIterator& rhs) { return rhs - lhs; }

		diff_t operator-(const RowIterator& other) const { return _ptr - other._ptr; }

		bool operator==(const RowIterator& other) const noexcept { return _ptr == other._ptr; }
		bool operator!=(const RowIterator& other) const noexcept { return _ptr != other._ptr; }
		bool operator< (const RowIterator& other) const noexcept { return _ptr <  other._ptr; }
		bool operator> (const RowIterator& other) const noexcept { return _ptr >  other._ptr; }
		bool operator<=(const RowIterator& other) const noexcept { return _ptr <= other._ptr; }
		bool operator>=(const RowIterator& other) const noexcept { return _ptr >= other._ptr; }

		RowProxy& operator[](const diff_t rhs) const { return *(_ptr + rhs * _len); }
		#pragma endregion
	};
	#pragma endregion

private:
	#pragma region Main Ctor
	explicit Map2D(const paramS rows, const paramS cols)
		: mRows{ rows }
		, mCols{ cols }
		, mSize{ static_cast<paramU>(rows * cols) }
		, pData{ std::make_unique<T[]>(mSize) }
	{}
	#pragma endregion

public:
	#pragma region Trivial Ctor
	Map2D() : Map2D{ 1, 1 } {}

	Map2D(const integral auto rows, const integral auto cols)
		: Map2D{
			std::max<paramS>(1, std::abs(rows)),
			std::max<paramS>(1, std::abs(cols))
		}
	{}
	#pragma endregion
	
	#pragma region Copy/Move Ctor
	Map2D(Map2D&&) = default; // move constructor
	Map2D(const Map2D& other) // copy constructor
		: Map2D{
			other.mRows,
			other.mCols
		}
	{
		std::copy(other.mBegin(), other.mEnd(), mBegin());
	}
	#pragma endregion

	#pragma region Move/Copy Assignment
	Map2D& operator=(Map2D&&) = default;   // move assignment
	Map2D& operator=(const Map2D& other) { // copy assignment
		if (this != &other && mSize == other.mSize) {
			std::copy(other.begin(), other.end(), begin());
			mRows = other.mRows; mCols = other.mCols;
		}
		return *this;
	}
	#pragma endregion

private:
	#pragma region negmod()
	auto negmod(const integral auto lhs, const integral auto rhs) const {
		const auto _mod{
			static_cast<std::ptrdiff_t>(lhs) %
			static_cast<std::ptrdiff_t>(rhs)
		};
		if (std::cmp_less(_mod, 0)) {
			return _mod + rhs;
		} else {
			return _mod;
		}
	}
	#pragma endregion

public:
	#pragma region makeView() :: Matrix
	/**
	 * @brief Creates a View of the original matrix data as a matrix of const T*.
	 *        Only the arrangement of the pointers can be modified.
	 * @returns Const matrix of pointers to data.
	 *
	 * @param[in] rows,cols :: Dimensions of the new View matrix.
	 *                         If 0, default dimensions of matrix are used.
	 * @param[in] posY,posX :: Dimension offset to apply on original matrix.
	 *
	 * @warning There are no limiters in place. You can repeat a pattern if you wish.
	 * @warning Elements in the View matrix must be dereferenced to be used.
	 */
	auto makeView(
		const integral auto rows = 0,
		const integral auto cols = 0,
		const integral auto posY = 0,
		const integral auto posX = 0
	) const requires arithmetic<T> {
		const auto nRows{ std::cmp_equal(rows, 0) ? mRows : static_cast<paramS>(std::abs(rows)) };
		const auto nCols{ std::cmp_equal(cols, 0) ? mCols : static_cast<paramS>(std::abs(cols)) };

		Map2D<const T*> obj;
		return obj.setView(this, nRows, nCols, posY, posX);
	}
	#pragma endregion
	
	#pragma region makeView() :: View
	/**
	 * @brief Creates a View from the pointers of another View matrix.
	 *        Only the arrangement of the pointers can be modified.
	 * @returns Const matrix of pointers to data.
	 *
	 * @param[in] rows,cols :: Dimensions of the new View matrix.
	 *                         If 0, default dimensions of matrix are used.
	 * @param[in] posY,posX :: Dimension offset to apply on original matrix.
	 *
	 * @warning There are no limiters in place. You can repeat a pattern if you wish.
	 * @warning Elements in the View matrix must be dereferenced to be used.
	 */
	auto makeView(
		const integral auto rows = 0,
		const integral auto cols = 0,
		const integral auto posY = 0,
		const integral auto posX = 0
	) const requires ar_pointer<T> {
		const auto nRows{ std::cmp_equal(rows, 0) ? mRows : static_cast<paramS>(std::abs(rows)) };
		const auto nCols{ std::cmp_equal(cols, 0) ? mCols : static_cast<paramS>(std::abs(cols)) };

		Map2D obj;
		return obj.setView(this, nRows, nCols, posY, posX);
	}
	#pragma endregion
	
	#pragma region setView() :: Matrix
	/**
	 * @brief Reseat a View using a matrix of original data.
	 * @returns Self reference for method chaining.
	 *
	 * @param[in] base*     :: Pointer to an object of matrix data.
	 * @param[in] rows,cols :: Dimensions of the new View matrix.
	 *                         If 0, default dimensions of matrix are used.
	 * @param[in] posY,posX :: Dimension offset to apply on original matrix.
	 *
	 * @warning There are no limiters in place. You can repeat a pattern if you wish.
	 * @warning Elements in the View matrix must be dereferenced to be used.
	 */
	Map2D& setView(
		const Map2D<underT>* const base,
		const integral auto rows = 0,
		const integral auto cols = 0,
		const integral auto posY = 0,
		const integral auto posX = 0
	) requires ar_pointer<T> {
		mRows = std::cmp_equal(rows, 0) ? mRows : static_cast<paramS>(std::abs(rows));
		mCols = std::cmp_equal(cols, 0) ? mRows : static_cast<paramS>(std::abs(cols));

		resizeWipe(mRows, mCols);
		for (paramS y{}; std::cmp_less(y, mRows); ++y) {
			const auto offsetY{ negmod(y + posY, base->lenY()) };
			for (paramS x{}; std::cmp_less(x, mCols); ++x) {
				const auto offsetX{ negmod(x + posX, base->lenX()) };
				at_raw(y, x) = &base->at_raw(offsetY, offsetX);
			}
		}
		return *this;
	}
	#pragma endregion
	
	#pragma region setView() :: View
	/**
	 * @brief Reseat a View from the pointers of another View matrix.
	 * @returns Self reference for method chaining.
	 *
	 * @param[in] base*     :: Pointer to an object of matrix data.
	 * @param[in] rows,cols :: Dimensions of the new View matrix.
	 *                         If 0, default dimensions of matrix are used.
	 * @param[in] posY,posX :: Dimension offset to apply on original matrix.
	 *
	 * @warning There are no limiters in place. You can repeat a pattern if you wish.
	 * @warning Elements in the View matrix must be dereferenced to be used.
	 */
	Map2D& setView(
		const Map2D<const underT*>* const base,
		const integral auto rows = 0,
		const integral auto cols = 0,
		const integral auto posY = 0,
		const integral auto posX = 0
	) requires ar_pointer<T> {
		mRows = std::cmp_equal(rows, 0) ? mRows : static_cast<paramS>(std::abs(rows));
		mCols = std::cmp_equal(cols, 0) ? mRows : static_cast<paramS>(std::abs(cols));

		resizeWipe(mRows, mCols);
		for (paramS y{}; std::cmp_less(y, mRows); ++y) {
			const auto offsetY{ negmod(y + posY, base->lenY()) };
			for (paramS x{}; std::cmp_less(x, mCols); ++x) {
				const auto offsetX{ negmod(x + posX, base->lenX()) };
				at_raw(y, x) = base->at_raw(offsetY, offsetX);
			}
		}
		return *this;
	}
	#pragma endregion
	
	#pragma region copyLinear() :: Matrix
	/**
	 * @brief Copies data from another matrix of the same type. As much
	 *        as it can fit or pull.
	 * @returns Self reference for method chaining.
	 * 
	 * @param[in] other :: Matrix object to copy from.
	 */
	Map2D& copyLinear(
		const Map2D& other
	) requires arithmetic<T> {
		const auto nSize{ std::min(mSize, other.mSize) };
		std::copy_n(other.mBegin(), nSize, mBegin());
		return *this;
	}
	#pragma endregion
	
	#pragma region copyLinear() :: C-style Array
	/**
	 * @brief Copies data from array pointer of the same type. As much
	 *        as it can fit or pull.
	 * @returns Self reference for method chaining.
	 *
	 * @param[in] other :: Pointer to array of data.
	 * @param[in] size  :: Amount of elements to copy.
	 * 
	 * @warning Cannot check if target array has sufficient data.
	 */
	Map2D& copyLinear(
		const T* const other,
		const integral auto size
	) requires arithmetic<T> {
		const auto nSize{ static_cast<paramU>(std::abs(size)) };
		std::copy_n(other, std::min(nSize, mSize), mBegin());
		return *this;
	}
	#pragma endregion
	
	#pragma region resize() :: Matrix (Arithmetic)
	/**
	 * @brief Resizes the matrix to new dimensions. Can either copy
	 *        existing data or wipe it.
	 * @return Self reference for method chaining.
	 * 
	 * @param[in] choice :: FALSE to clear data, TRUE to copy it instead.
	 * @param[in] rows   :: Total rows of the new matrix.    (min: 1)
	 * @param[in] cols   :: Total columns of the new matrix. (min: 1)
	 */
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
	#pragma endregion

private:
	#pragma region resizeCopy()
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
	#pragma endregion
	
	#pragma region resizeWipe()
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
	#pragma endregion

public:
	#pragma region wipeAll() :: Matrix (Arithmetic)
	/**
	 * @brief Wipes all of the matrix's data.
	 * @return Self reference for method chaining.
	 */
	Map2D& wipeAll() requires arithmetic<T> {
		std::fill(mBegin(), mEnd(), T());
		return *this;
	}
	#pragma endregion
	
	#pragma region wipe() :: Matrix (Arithmetic)
	/**
	 * @brief Wipes the matrix's data in a given direction.
	 * @return Self reference for method chaining.
	 *
	 * @param[in] rows :: Total rows to wipe. Directional.
	 * @param[in] cols :: Total columns to wipe. Directional.
	 *
	 * @warning The sign of the params control the application direction.
	 * @warning If the params exceed row/column length, all row data is wiped.
	 */
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
	#pragma endregion
	
	#pragma region rotate() :: Matrix + View
	/**
	 * @brief Rotates the matrix's data in a given direction.
	 * @return Self reference for method chaining.
	 *
	 * @param[in] rows :: Total rows to rotate. Directional.
	 * @param[in] cols :: Total columns to rotate. Directional.
	 *
	 * @warning The sign of the params control the application direction.
	 */
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
			for (auto& row : *this) {
				row.rotate(cols);
			}
		}
		return *this;
	}
	#pragma endregion
	
	#pragma region shift() :: Matrix (Arithmetic)
	/**
	 * @brief Shifts the matrix's data in a given direction. Combines the
	 *        functionality of rotating and wiping.
	 * @return Self reference for method chaining.
	 *
	 * @param[in] rows :: Total rows to shift. Directional.
	 * @param[in] cols :: Total columns to shift. Directional.
	 *
	 * @warning The sign of the params control the application direction.
	 * @warning If the params exceed row/column length, all row data is wiped.
	 */
	Map2D& shift(
		const integral auto rows,
		const integral auto cols
	) requires arithmetic<T> {
		if (std::cmp_less(std::abs(rows), mRows) && std::cmp_less(std::abs(cols), mCols)) {
			rotate(rows, cols);
		}
		return wipe(rows, cols);
	}
	#pragma endregion
	
	#pragma region shiftBit() :: Matrix (Integral)
	/**
	 * @brief Shifts the matrix's data in a bitwise manner in a given direction.
	 * @return Self reference for method chaining.
	 *
	 * @param[in] rows  :: Total row positions to shift. Directional.
	 * @param[in] cols  :: Total column bit positions to shift. Directional.
	 * @param[in] limit :: Apply a bit limiter. Defaults to T size, optional.
	 *
	 * @warning The sign of the params control the application direction.
	 * @warning If the params exceed row/column length, all row data is wiped.
	 * @warning Row bit shifting is limited in distance to matrix T type.
	 */
	Map2D& shiftBit(
		const integral auto rows,
		const integral auto cols,
		const paramU limit = sizeof(T) * 8
	) requires integral<T> {
		if (std::cmp_less(std::abs(rows), mRows)) {
			shift(rows, 0);
		}
		for (auto& row : *this) {
			row.shiftBit(cols, limit);
		}
		return *this;
	}
	#pragma endregion
	
	#pragma region shiftBitMask() :: Matrix (Integral)
	/**
	 * @brief Shifts the matrix's data in a marked bitwise manner in a given direction.
	 * @return Self reference for method chaining.
	 *
	 * @param[in] rows  :: Total row positions to shift. Directional.
	 * @param[in] cols  :: Total column bit positions to shift. Directional.
	 * @param[in] mask  :: Bit mask to control the shift pattern.
	 * @param[in] limit :: Apply a bit limiter. Defaults to T size, optional.
	 *
	 * @warning The sign of the params control the application direction.
	 * @warning If the params exceed row/column length, all row data is wiped.
	 * @warning Row bit shifting is limited in distance to matrix T type.
	 */
	Map2D& shiftBitMask(
		const integral auto rows,
		const integral auto cols,
		const integral auto mask,
		const paramU limit = sizeof(T) * 8
	) requires integral<T> {
		if (std::cmp_not_equal(rows, 0)) {
			const auto limiter{ (1 << limit) - 1 };

			if (std::cmp_less(rows, 0)) {
				for (auto Y{ 0 }; Y < mRows; ++Y) {
					(*this)[Y] &= ~mask;
					if (Y >= mRows + rows) continue;
					(*this)[Y] |= (*this)[Y - rows].clone() & (mask & limiter);
				}
			} else {
				for (auto Y{ mRows - 1 }; Y >= 0; --Y) {
					(*this)[Y] &= ~mask;
					if (Y < rows) continue;
					(*this)[Y] |= (*this)[Y - rows].clone() & (mask & limiter);
				}
			}
		}
		for (auto& row : *this) {
			row.shiftBitMask(cols, mask, limit);
		}
		return *this;
	}
	#pragma endregion
	
	#pragma region reverse() :: Matrix + View
	/**
	 * @brief Reverses the matrix's data.
	 * @return Self reference for method chaining.
	 */
	Map2D& reverse() {
		std::reverse(mBegin(), mEnd());
		return *this;
	}
	#pragma endregion
	
	#pragma region reverseY() :: Matrix + View
	/**
	 * @brief Reverses the matrix's data in row order.
	 * @return Self reference for method chaining.
	 */
	Map2D& reverseY() {
		for (auto row{ 0 }; std::cmp_less(row, mRows / 2); ++row) {
			(*this)[row].swap((*this)[mRows - row - 1]);
		}
		return *this;
	}
	#pragma endregion
	
	#pragma region reverseX() :: Matrix + View
	/**
	 * @brief Reverses the matrix's data in column order.
	 * @return Self reference for method chaining.
	 */
	Map2D& reverseX() {
		for (auto& row : *this) {
			std::reverse(row.begin(), row.end());
		}
		return *this;
	}
	#pragma endregion
	
	#pragma region transpose() :: Matrix + View
	/**
	 * @brief Transposes the matrix's data. Works with rectangular dimensions.
	 * @return Self reference for method chaining.
	 */
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
	#pragma endregion

private:
	#pragma region Accessor Bounds Checkers
	paramS checkRowBounds(const integral auto row) const {
		if (std::cmp_less(row, -mRows) || std::cmp_greater_equal(row, mRows)) {
			throw std::out_of_range("row index out of range");
		}
		if (std::cmp_less(row, 0)) return row + mRows;
		else return row;
	}
	paramS checkColBounds(const integral auto col) const {
		if (std::cmp_less(col, -mCols) || std::cmp_greater_equal(col, mCols)) {
			throw std::out_of_range("column index out of range");
		}
		if (std::cmp_less(col, 0)) return col + mCols;
		else return col;
	}
	#pragma endregion

public:
	#pragma region Accessors
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
	#pragma endregion

private:
	#pragma region Raw Iterator begin/end
	T* mBegin()  const noexcept { return pData.get(); }
	T* mEnd()    const noexcept { return pData.get() + mSize; }

	T* mBeginR() const noexcept { return mEnd() - 1; }
	T* mEndR()   const noexcept { return mBegin() - 1; }
	#pragma endregion

public:
	#pragma region Iterator begin/end
	RowIterator begin() const noexcept { return RowIterator(mBegin(), mCols); }
	RowIterator end()   const noexcept { return RowIterator(mEnd(), mCols); }

	RowIterator rbegin() const noexcept { return RowIterator(mBeginR(), mCols); }
	RowIterator rend()   const noexcept { return RowIterator(mEndR(), mCols); }

	const RowIterator cbegin() const noexcept { return begin(); }
	const RowIterator cend()   const noexcept { return end(); }

	const RowIterator crbegin() const noexcept { return rbegin(); }
	const RowIterator crend()   const noexcept { return rend(); }
	#pragma endregion
};
#pragma endregion
