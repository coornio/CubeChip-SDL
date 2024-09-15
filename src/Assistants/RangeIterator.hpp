/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <cstddef>
#include <span>

#pragma region RangeProxy Class
template <typename T>
class RangeProxy {
    using diff_t = std::ptrdiff_t;
    using size_t = std::size_t;

protected:
	T*           mBegin;
	const diff_t mLength;

public:
	auto size() const { return mLength; }

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
	RangeProxy(
		T* const     begin,
		const diff_t length
	) noexcept
		: mBegin { begin  }
		, mLength{ length }
	{}
	#pragma endregion

public:
	#pragma region Accessors
	/* bounds-checked accessors, reverse indexing allowed */

	T& at(const diff_t idx) {
		return *(begin() + col + (col < 0 ? mLength : 0);
	}
	const T& at(const diff_t idx) const {
		return *(begin() + col + (col < 0 ? mLength : 0);
	}

	/* unsafe accessors */

	T& operator[](const diff_t idx) {
		return *(begin() + idx);
	}
	const T& operator[](const diff_t idx) const {
		return *(begin() + idx);
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

#pragma region RangeIterator Class
template <typename T>
class RangeIterator final : private RangeProxy<T> {
	using diff_t = std::ptrdiff_t;
    using size_t = std::size_t;

public:
	#pragma region Ctor
	RangeIterator(
		T* const     begin,
		const diff_t length
	) noexcept
		: RangeProxy<T>{ begin, length }
	{}
	#pragma endregion

public:
	#pragma region Iterator Overloads
	RangeProxy<T>& operator* () noexcept { return *this; }
	RangeProxy<T>* operator->() noexcept { return  this; }

	RangeIterator& operator++() noexcept { this->mBegin += this->mLength; return *this; }
	RangeIterator& operator--() noexcept { this->mBegin -= this->mLength; return *this; }

	RangeIterator operator++(int) noexcept { auto tmp{ *this }; this->mBegin += this->mLength; return tmp; }
	RangeIterator operator--(int) noexcept { auto tmp{ *this }; this->mBegin -= this->mLength; return tmp; }
		
	RangeIterator  operator+ (const diff_t rhs) const { return RangeIterator(this->mBegin + rhs * this->mLength, this->mLength); }
	RangeIterator  operator- (const diff_t rhs) const { return RangeIterator(this->mBegin - rhs * this->mLength, this->mLength); }
		
	RangeIterator& operator+=(const diff_t rhs) { this->mBegin += rhs * this->mLength; return *this; }
	RangeIterator& operator-=(const diff_t rhs) { this->mBegin -= rhs * this->mLength; return *this; }

	friend RangeIterator operator+(const diff_t lhs, const RangeIterator& rhs) { return rhs + lhs; }
	friend RangeIterator operator-(const diff_t lhs, const RangeIterator& rhs) { return rhs - lhs; }

	diff_t operator-(const RangeIterator& other) const { return this->mBegin - other.mBegin; }

	bool operator==(const RangeIterator& other) const noexcept { return this->mBegin == other.mBegin; }
	bool operator!=(const RangeIterator& other) const noexcept { return this->mBegin != other.mBegin; }
    bool operator< (const RangeIterator& other) const noexcept { return this->mBegin <  other.mBegin; }
	bool operator> (const RangeIterator& other) const noexcept { return this->mBegin >  other.mBegin; }
	bool operator<=(const RangeIterator& other) const noexcept { return this->mBegin <= other.mBegin; }
	bool operator>=(const RangeIterator& other) const noexcept { return this->mBegin >= other.mBegin; }

	RangeProxy<T>& operator[](const diff_t rhs) const { return *(this->mBegin + rhs * this->mLength); }
	#pragma endregion
};
#pragma endregion
