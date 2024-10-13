/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Concepts.hpp"

#include <span>
#include <cmath>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <execution>
#include <vector>
#include <stdexcept>
#include <utility>

#pragma region MapRow Class
template<typename T> requires arithmetic<T> || ar_pointer<T>
class MapRow : public std::vector<T> {
	using std::vector<T>::vector;
	using paramU = std::size_t;

public:
	#pragma region operator +=
	constexpr MapRow& operator+=(
		const MapRow& other
	) requires arithmetic<T> {
		const auto len{ std::min(this->size(), other.size()) };
		for (paramU i{ 0 }; i < len; ++i) {
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
		for (paramU i{ 0 }; i < len; ++i) {
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
		for (paramU i{ 0 }; i < len; ++i) {
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
		for (paramU i{ 0 }; i < len; ++i) {
			if (other[i] == 0) {
				throw std::domain_error("division by zero");
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
			if (value == 0) {
				throw std::domain_error("division by zero");
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
		for (paramU i{ 0 }; i < len; ++i) {
			if (other[i] == 0) {
				throw std::domain_error("division by zero");
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
			if (value == 0) {
				throw std::domain_error("division by zero");
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
		for (paramU i{ 0 }; i < len; ++i) {
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
		for (paramU i{ 0 }; i < len; ++i) {
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
		for (paramU i{ 0 }; i < len; ++i) {
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
		for (paramU i{ 0 }; i < len; ++i) {
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
		for (paramU i{ 0 }; i < len; ++i) {
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
		for (paramU i{ 0 }; i < len; ++i) {
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
		for (paramU i{ 0 }; i < len; ++i) {
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
		for (paramU i{ 0 }; i < len; ++i) {
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
		for (paramU i{ 0 }; i < len; ++i) {
			if (other[i] == 0) {
				throw std::domain_error("division by zero");
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
			if (value == 0) {
				throw std::domain_error("division by zero");
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
		for (paramU i{ 0 }; i < len; ++i) {
			if (other[i] == 0) {
				throw std::domain_error("division by zero");
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
			if (value == 0) {
				throw std::domain_error("division by zero");
			} else {
				elem = std::fmod(elem, value);
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
		for (paramU i{ 0 }; i < len; ++i) {
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
		for (paramU i{ 0 }; i < len; ++i) {
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
		for (paramU i{ 0 }; i < len; ++i) {
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
	using paramS = std::int32_t;
	using paramU = std::size_t;
	using underT = std::remove_const_t<std::remove_pointer_t<T>>;

	paramS mRows;
	paramS mCols;
	std::unique_ptr<T[]> pData;

public:
	paramU size() const noexcept { return mRows * mCols; }
	paramS lenX() const noexcept { return mCols; }
	paramS lenY() const noexcept { return mRows; }

	T& front() { return pData[0]; }
	T& back()  { return pData[size() - 1]; }
	T* data()  { return pData.get(); }

	const T& front() const { return pData[0]; }
	const T& back()  const { return pData[size() - 1]; }
	const T* data()  const { return pData.get(); }

	auto span()       { return std::span<      T>{ data(), size() }; }
	auto span() const { return std::span<const T>{ data(), size() }; }

public:
	#pragma region Raw Accessors
	auto at_raw(const paramU idx)
	-> T& {
		assert(idx < size() && "at_raw() index out of bounds");
		return pData[idx];
	}

	auto at_raw(const paramU idx) const
	-> const T& {
		assert(idx < size() && "at_raw() index out of bounds");
		return pData[idx];
	}

	auto at_raw(const paramU row, const paramU col)
	-> T& {
		assert(rowValid(row) && "at_raw() row index out of bounds");
		assert(colValid(col) && "at_raw() col index out of bounds");
		return pData[row * mCols + col];
	}

	auto at_raw(const paramU row, const paramU col) const
	-> const T& {
		assert(rowValid(row) && "at_raw() row index out of bounds");
		assert(colValid(col) && "at_raw() col index out of bounds");
		return pData[row * mCols + col];
	}

	auto at_wrap(const paramU row, const paramU col)
	-> T&
	{ return pData[(row % mRows) * mCols + (col % mCols)]; }

	auto at_wrap(const paramU row, const paramU col) const
	-> const T&
	{ return pData[(row % mRows) * mCols + (col % mCols)]; }
	#pragma endregion

private:
	#pragma region RowProxy Class
	class RowProxy {
	protected:
		T*           mBegin;
		const paramS mLength;

	public:
		paramS size() const { return mLength; }

		T& front() { return mBegin[0]; }
		T& back()  { return mBegin[mLength - 1]; }
		T* data()  { return mBegin; }

		const T& front() const { return mBegin[0]; }
		const T& back()  const { return mBegin[mLength - 1]; }
		const T* data()  const { return mBegin; }

		auto span()       { return std::span<      T>{ mBegin, mLength }; }
		auto span() const { return std::span<const T>{ mBegin, mLength }; }

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
			return MapRow<T>(mBegin, mBegin + mLength);
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
			std::fill(
				std::execution::unseq,
				begin(), end(), T()
			);
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
			if (!colValidAbs(cols)) {
				wipeAll();
			} else {
				const auto _cols{ static_cast<paramS>(cols) };
				if (_cols) {
					if (_cols < 0) {
						std::fill(
							std::execution::unseq,
							end() + _cols, end(), T()
						);
					} else {
						std::fill(
							std::execution::unseq,
							begin(), begin() + _cols, T()
						);
					}
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
			const auto _cols{ static_cast<paramS>(cols) };
			if (_cols) {
				if (_cols < 0) {
					std::rotate(
						std::execution::unseq,
						begin(), begin() + std::abs(_cols) % mLength, end()
					);
				} else {
					std::rotate(
						std::execution::unseq,
						begin(), end() - std::abs(_cols) % mLength, end()
					);
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
			if (colValidAbs(cols)) {
				rotate(cols);
			}
			return wipe(cols);
		}
		#pragma endregion
		
		#pragma region reverse() :: Matrix[R] + View[R]
		/**
		 * @brief Reverses the row's data.
		 * @return Self reference for method chaining.
		 */
		RowProxy& reverse() {
			std::reverse(
				std::execution::unseq,
				begin(), end()
			);
			return *this;
		}
		#pragma endregion

	private:
		#pragma region Accessor Bounds Checker
		paramS checkColBounds(const paramS col) const {
			if (col < -mLength || col > mLength) {
				throw std::out_of_range("proxy at() col index out of range");
			}
			return col + (col < 0 ? mLength : 0);
		}
		
		bool colValid(const paramU idx) const noexcept {
			return idx < static_cast<paramU>(mLength);
		}
		bool colValidAbs(const integral auto idx) const noexcept {
			return std::abs(static_cast<paramS>(idx)) < mLength;
		}
		#pragma endregion

	public:
		#pragma region Accessors
		/* bounds-checked accessors, reverse indexing allowed */

		auto at(const integral auto col)
		-> T&
		{ return *(begin() + checkColBounds(static_cast<paramS>(col))); }

		auto at(const integral auto col) const
		-> const T&
		{ return *(begin() + checkColBounds(static_cast<paramS>(col))); }

		/* unsafe accessors */

		auto operator[](const paramU col)
		-> T& {
			assert(colValid(col) && "proxy operator[] col index out of bounds");
			return *(begin() + col);
		}

		auto operator[](const paramU col) const
		-> const T& {
			assert(colValid(col) && "proxy operator[] col index out of bounds");
			return *(begin() + col);
		}
		#pragma endregion

	public:
		#pragma region operator =
		RowProxy& operator=(
			const arithmetic auto& value
		) requires arithmetic<T> {
			std::fill(
				std::execution::unseq,
				begin(), end(), static_cast<T>(value)
			);
			return *this;
		}
		RowProxy& operator=(
			const RowProxy& other
		) requires arithmetic<T> {
			if (this == &other) [[unlikely]] return *this;
			const auto len{ std::min(other.size(), static_cast<paramU>(mLength)) };
			std::copy(
				std::execution::unseq,
				other.begin(), other.begin() + len, mBegin
			);
			return *this;
		}
		RowProxy& operator=(
			MapRow<T>&& other
		) requires arithmetic<T> {
			const auto len{ std::min(other.size(), static_cast<paramU>(mLength)) };
			std::move(
				std::execution::unseq,
				other.begin(), other.begin() + len, mBegin
			);
			return *this;
		}
		RowProxy& operator=(
			const MapRow<T>& other
		) requires arithmetic<T> {
			const auto len{ std::min(other.size(), static_cast<paramU>(mLength)) };
			std::copy(
				std::execution::unseq,
				other.begin(), other.begin() + len, mBegin
			);
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
			for (paramS i{ 0 }; i < len; ++i) {
				(*this)[i] += other[i];
			}
			return *this;
		}
		RowProxy& operator+=(
			const MapRow<T>& other
		) requires arithmetic<T> {
			const auto len{ std::min(other.size(), static_cast<paramU>(mLength)) };
			for (paramU i{ 0 }; i < len; ++i) {
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
			for (paramS i{ 0 }; i < len; ++i) {
				(*this)[i] -= other[i];
			}
			return *this;
		}
		RowProxy& operator-=(
			const MapRow<T>& other
		) requires arithmetic<T> {
			const auto len{ std::min(other.size(), static_cast<paramU>(mLength)) };
			for (paramU i{ 0 }; i < len; ++i) {
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
			for (paramS i{ 0 }; i < len; ++i) {
				(*this)[i] *= other[i];
			}
			return *this;
		}
		RowProxy& operator*=(
			const MapRow<T>& other
		) requires arithmetic<T> {
			const auto len{ std::min(other.size(), static_cast<paramU>(mLength)) };
			for (paramU i{ 0 }; i < len; ++i) {
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
				if (value == 0) {
					throw std::domain_error("division by zero");
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
			for (paramS i{ 0 }; i < len; ++i) {
				if (other[i] == 0) {
					throw std::domain_error("division by zero");
				} else {
					(*this)[i] /= other[i];
				}
			}
			return *this;
		}
		RowProxy& operator/=(
			const MapRow<T>& other
		) requires arithmetic<T> {
			const auto len{ std::min(other.size(), static_cast<paramU>(mLength)) };
			for (paramU i{ 0 }; i < len; ++i) {
				if (other[i] == 0) {
					throw std::domain_error("division by zero");
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
				if (value == 0) {
					throw std::domain_error("division by zero");
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
			for (paramS i{ 0 }; i < len; ++i) {
				if (other[i] == 0) {
					throw std::domain_error("division by zero");
				} else {
					(*this)[i] = std::fmod((*this)[i], other[i]);
				}
			}
			return *this;
		}
		RowProxy& operator%=(
			const MapRow<T>& other
		) requires arithmetic<T> {
			const auto len{ std::min(other.size(), static_cast<paramU>(mLength)) };
			for (paramU i{ 0 }; i < len; ++i) {
				if (other[i] == 0) {
					throw std::domain_error("division by zero");
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
			for (paramS i{ 0 }; i < len; ++i) {
				(*this)[i] &= other[i];
			}
			return *this;
		}
		RowProxy& operator&=(
			const MapRow<T>& other
		) requires integral<T> {
			const auto len{ std::min<paramU>(other.size(), mLength) };
			for (paramU i{ 0 }; i < len; ++i) {
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
			for (paramS i{ 0 }; i < len; ++i) {
				(*this)[i] |= other[i];
			}
			return *this;
		}
		RowProxy& operator|=(
			const MapRow<T>& other
		) requires integral<T> {
			const auto len{ std::min(other.size(), static_cast<paramU>(mLength)) };
			for (paramU i{ 0 }; i < len; ++i) {
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
			for (paramS i{ 0 }; i < len; ++i) {
				(*this)[i] ^= other[i];
			}
			return *this;
		}
		RowProxy& operator^=(
			const MapRow<T>& other
		) requires integral<T> {
			const auto len{ std::min(other.size(), static_cast<paramU>(mLength)) };
			for (paramU i{ 0 }; i < len; ++i) {
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
			for (paramS i{ 0 }; i < len; ++i) {
				(*this)[i] <<= other[i];
			}
			return *this;
		}
		RowProxy& operator<<=(
			const MapRow<T>& other
		) requires integral<T> {
			const auto len{ std::min(other.size(), static_cast<paramU>(mLength)) };
			for (paramU i{ 0 }; i < len; ++i) {
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
			for (paramS i{ 0 }; i < len; ++i) {
				(*this)[i] >>= other[i];
			}
			return *this;
		}
		RowProxy& operator>>=(
			const MapRow<T>& other
		) requires integral<T> {
			const auto len{ std::min(other.size(), static_cast<paramU>(mLength)) };
			for (paramU i{ 0 }; i < len; ++i) {
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

	public:
		#pragma region Ctor
		explicit RowIterator(
			T* const     begin,
			const paramS length
		) noexcept
			: RowProxy{ begin, length }
		{}
		#pragma endregion

	public:
		#pragma region Iterator Overloads
		RowProxy& operator* () noexcept { return *this; }
		RowProxy* operator->() noexcept { return  this; }

		RowIterator& operator++() noexcept { this->mBegin += this->mLength; return *this; }
		RowIterator& operator--() noexcept { this->mBegin -= this->mLength; return *this; }

		RowIterator operator++(int) noexcept { auto tmp{ *this }; this->mBegin += this->mLength; return tmp; }
		RowIterator operator--(int) noexcept { auto tmp{ *this }; this->mBegin -= this->mLength; return tmp; }
		
		RowIterator  operator+ (const diff_t rhs) const { return RowIterator(this->mBegin + rhs * this->mLength, this->mLength); }
		RowIterator  operator- (const diff_t rhs) const { return RowIterator(this->mBegin - rhs * this->mLength, this->mLength); }
		
		RowIterator& operator+=(const diff_t rhs) { this->mBegin += rhs * this->mLength; return *this; }
		RowIterator& operator-=(const diff_t rhs) { this->mBegin -= rhs * this->mLength; return *this; }

		friend RowIterator operator+(const diff_t lhs, const RowIterator& rhs) { return rhs + lhs; }
		friend RowIterator operator-(const diff_t lhs, const RowIterator& rhs) { return rhs - lhs; }

		diff_t operator-(const RowIterator& other) const { return this->mBegin - other.mBegin; }

		bool operator==(const RowIterator& other) const noexcept { return this->mBegin == other.mBegin; }
		bool operator!=(const RowIterator& other) const noexcept { return this->mBegin != other.mBegin; }
		bool operator< (const RowIterator& other) const noexcept { return this->mBegin <  other.mBegin; }
		bool operator> (const RowIterator& other) const noexcept { return this->mBegin >  other.mBegin; }
		bool operator<=(const RowIterator& other) const noexcept { return this->mBegin <= other.mBegin; }
		bool operator>=(const RowIterator& other) const noexcept { return this->mBegin >= other.mBegin; }

		RowProxy& operator[](const diff_t rhs) const { return *(this->mBegin + rhs * this->mLength); }
		#pragma endregion
	};
	#pragma endregion

public:
	#pragma region Trivial Ctor
	Map2D() : Map2D{ 1, 1 } {}

	Map2D(const paramS rows, const paramS cols)
		: mRows{ std::max(1, static_cast<paramS>(std::abs(rows))) }
		, mCols{ std::max(1, static_cast<paramS>(std::abs(cols))) }
		, pData{ std::make_unique<T[]>(mRows * mCols) }
	{}
	Map2D(const paramU rows, const paramU cols)
		: mRows{ std::max(1, static_cast<paramS>(rows)) }
		, mCols{ std::max(1, static_cast<paramS>(cols)) }
		, pData{ std::make_unique<T[]>(mRows * mCols) }
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
		std::copy(
			std::execution::unseq,
			other.mBegin(), other.mEnd(), mBegin()
		);
	}
	#pragma endregion

	#pragma region Move/Copy Assignment
	Map2D& operator=(Map2D&&) = default;   // move assignment
	Map2D& operator=(const Map2D& other) { // copy assignment
		if (this != &other && size() == other.size()) {
			std::copy(
				std::execution::unseq,
				other.mBegin(), other.mEnd(), mBegin()
			);
			mRows = other.lenY(); mCols = other.lenX();
		}
		return *this;
	}
	#pragma endregion

private:
	#pragma region negmod()
	auto negmod(const integral auto lhs, const integral auto rhs) const {
		const auto _mod{
			static_cast<paramS>(lhs) %
			static_cast<paramS>(rhs)
		};
		if (_mod < 0) {
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
		const auto _rows{ static_cast<paramS>(std::abs(rows)) };
		const auto _cols{ static_cast<paramS>(std::abs(cols)) };

		const auto nRows = !_rows ? mRows : _rows;
		const auto nCols = !_cols ? mRows : _cols;

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
		const auto _rows{ static_cast<paramS>(std::abs(rows)) };
		const auto _cols{ static_cast<paramS>(std::abs(cols)) };

		const auto nRows = !_rows ? mRows : _rows;
		const auto nCols = !_cols ? mRows : _cols;

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
		const auto _rows{ static_cast<paramS>(std::abs(rows)) };
		const auto _cols{ static_cast<paramS>(std::abs(cols)) };

		mRows = !_rows ? mRows : _rows;
		mCols = !_cols ? mRows : _cols;
		resizeWipe(mRows, mCols);

		for (paramS y{}; y < mRows; ++y) {
			const auto offsetY{ negmod(y + posY, base->lenY()) };
			for (paramS x{}; x < mCols; ++x) {
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
		const auto _rows{ static_cast<paramS>(std::abs(rows)) };
		const auto _cols{ static_cast<paramS>(std::abs(cols)) };

		mRows = !_rows ? mRows : _rows;
		mCols = !_cols ? mRows : _cols;
		resizeWipe(mRows, mCols);

		for (paramS y{}; y < mRows; ++y) {
			const auto offsetY{ negmod(y + posY, base->lenY()) };
			for (paramS x{}; x < mCols; ++x) {
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
		const auto _len{ std::min(size(), other.size())};
		std::copy_n(
			std::execution::unseq,
			other.mBegin(), _len, mBegin()
		);
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
		const integral auto len
	) requires arithmetic<T> {
		const auto _len{ static_cast<paramU>(std::abs(len)) };
		std::copy_n(
			std::execution::unseq,
			other, std::min(_len, size()), mBegin()
		);
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
	 * 
	 * @warning A 'rows'/'cols' of 0 will default to current size.
	 */
	Map2D& resize(
		const bool choice_copy,
		const integral auto rows,
		const integral auto cols
	) requires arithmetic<T> {
		auto nRows{ static_cast<paramS>(std::abs(rows)) };
		auto nCols{ static_cast<paramS>(std::abs(cols)) };

		if (!nRows) nRows = mRows;
		if (!nCols) nCols = mCols;

		if (nRows == mRows && nCols == mCols) {
			if (choice_copy) {
				return *this;
			} else {
				return wipeAll();
			}
		} else {
			if (choice_copy) {
				return resizeCopy(nRows, nCols);
			} else {
				return resizeWipe(nRows, nCols);
			}
		}
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

		auto pCopy{ std::make_unique<T[]>(rows * cols) };

		for (auto row{ 0 }; row < minRows; ++row) {
			const auto srcIdx{ pData.get() + row * mCols };
			const auto dstIdx{ pCopy.get() + row * cols };
			std::move(
				std::execution::unseq,
				srcIdx, srcIdx + minCols, dstIdx
			);
		}

		mRows = rows;
		mCols = cols;

		pData = std::move(pCopy);
		return *this;
	}
	#pragma endregion
	
	#pragma region resizeWipe()
	Map2D& resizeWipe(
		const paramS rows,
		const paramS cols
	) {
		mRows = rows;
		mCols = cols;

		pData = std::make_unique<T[]>(size());
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
		std::fill(
			std::execution::unseq,
			mBegin(), mEnd(), T()
		);
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
		if (!rowValidAbs(rows) || !colValidAbs(cols)) {
			wipeAll();
		} else {
			const auto _rows{ static_cast<paramS>(rows) };
			const auto _cols{ static_cast<paramS>(cols) };
			if (_rows) {
				if (_rows < 0) {
					std::fill(
						std::execution::unseq,
						mEnd() + _rows * mCols, mEnd(), T()
					);
				} else {
					std::fill(
						std::execution::unseq,
						mBegin(), mBegin() + _rows * mCols, T()
					);
				}
			}
			if (_cols) {
				for (auto& row : *this) {
					row.wipe(_cols);
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
		const auto _rows{ static_cast<paramS>(rows) };
		const auto _cols{ static_cast<paramS>(cols) };
		if (_rows % mRows) {
			if (_rows < 0) {
				std::rotate(
					std::execution::unseq,
					mBegin(), mBegin() - _rows * mCols, mEnd()
				);
			} else {
				std::rotate(
					std::execution::unseq,
					mBegin(), mEnd() - _rows * mCols, mEnd()
				);
			}
		}
		if (_cols % mCols) {
			for (auto& row : *this) {
				row.rotate(_cols);
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
		if (rowValidAbs(rows) && colValidAbs(cols)) {
			rotate(rows, cols);
		}
		return wipe(rows, cols);
	}
	#pragma endregion
	
	#pragma region reverse() :: Matrix + View
	/**
	 * @brief Reverses the matrix's data.
	 * @return Self reference for method chaining.
	 */
	Map2D& reverse() {
		std::reverse(
			std::execution::unseq,
			mBegin(), mEnd()
		);
		return *this;
	}
	#pragma endregion
	
	#pragma region reverseY() :: Matrix + View
	/**
	 * @brief Reverses the matrix's data in row order.
	 * @return Self reference for method chaining.
	 */
	Map2D& reverseY() {
		for (auto row{ 0 }; row < mRows / 2; ++row) {
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
			std::reverse(
				std::execution::unseq,
				row.begin(), row.end()
			);
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
		if (mRows > 1 || mCols > 1) {
			for (paramU a{ 1 }, b{ 1 }; a < size() - 1; b = ++a) {
				do {
					b = (b % mRows) * mCols + (b / mRows);
				} while (b < a);

				if (b != a) std::iter_swap(mBegin() + a, mBegin() + b);
			}
		}
		std::swap(mRows, mCols);
		return *this;
	}
	#pragma endregion

private:
	#pragma region Accessor Bounds Checkers
	paramS checkRowBounds(const paramS row) const {
		if (row < -mRows || row >= mRows) {
			throw std::out_of_range("at() row index out of range");
		}
		return row + (row < 0 ? mRows : 0);
	}
	paramS checkColBounds(const paramS col) const {
		if (col < -mCols || col >= mCols) {
			throw std::out_of_range("at() col index out of range");
		}
		return col + (col < 0 ? mCols : 0);
	}

	bool rowValid(const paramU idx) const noexcept {
		return idx < static_cast<paramU>(mRows);
	}
	bool colValid(const paramU idx) const noexcept {
		return idx < static_cast<paramU>(mCols);
	}
	bool rowValidAbs(const integral auto idx) const noexcept {
		return std::abs(static_cast<paramS>(idx)) < mRows;
	}
	bool colValidAbs(const integral auto idx) const noexcept {
		return std::abs(static_cast<paramS>(idx)) < mCols;
	}
	#pragma endregion

public:
	#pragma region Accessors
	/* bounds-checked accessors, reverse indexing allowed */

	auto at(const integral auto row, const integral auto col)
	-> T&
	{ return at_raw(checkRowBounds(static_cast<paramS>(row)), checkColBounds(static_cast<paramS>(col))); }

	auto at(const integral auto row, const integral auto col) const
	-> const T&
	{ return at_raw(checkRowBounds(static_cast<paramS>(row)), checkColBounds(static_cast<paramS>(col))); }

	auto at(const integral auto row)
	-> RowProxy
	{ return RowProxy(mBegin() + checkRowBounds(static_cast<paramS>(row)) * mCols, mCols); }

	auto at(const integral auto row) const
	-> const RowProxy
	{ return RowProxy(mBegin() + checkRowBounds(static_cast<paramS>(row)) * mCols, mCols); }

	/* unsafe accessors */

	auto operator() (const paramU row, const paramU col)
	-> T& {
		assert(rowValid(row) && "operator() row index out of bounds");
		assert(colValid(col) && "operator() col index out of bounds");
		return at_raw(row, col);
	}

	auto operator() (const paramU row, const paramU col) const
	-> const T& {
		assert(rowValid(row) && "operator() row index out of bounds");
		assert(colValid(col) && "operator() col index out of bounds");
		return at_raw(row, col);
	}

	auto operator[] (const paramU row)
	-> RowProxy {
		assert(rowValid(row) && "operator[] row index out of bounds");
		return RowProxy(mBegin() + row * mCols, mCols);
	}

	auto operator[] (const paramU row) const
	-> const RowProxy {
		assert(rowValid(row) && "operator[] row index out of bounds");
		return RowProxy(mBegin() + row * mCols, mCols);
	}
	#pragma endregion

private:
	#pragma region Raw Iterators (private)
	T* mBegin()  const noexcept { return pData.get(); }
	T* mEnd()    const noexcept { return pData.get() + size(); }

	T* mBeginR() const noexcept { return mEnd() - 1; }
	T* mEndR()   const noexcept { return mBegin() - 1; }
	#pragma endregion

public:
	#pragma region Raw Iterators (public)
	T* raw_begin() const noexcept { return mBegin(); }
	T* raw_end()   const noexcept { return mEnd(); }

	T* raw_rbegin() const noexcept { return mBeginR(); }
	T* raw_rend()   const noexcept { return mEndR(); }

	const T* raw_cbegin() const noexcept { return raw_begin(); }
	const T* raw_cend()   const noexcept { return raw_end(); }

	const T* raw_crbegin() const noexcept { return raw_rbegin(); }
	const T* raw_crend()   const noexcept { return raw_rend(); }
	#pragma endregion

public:
	#pragma region RowIterator Iterators
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
