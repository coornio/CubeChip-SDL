/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include <variant>
#include <cstdint>
#include <concepts>
#include <utility>
#include <type_traits>
#include <unordered_map>

/*==================================================================*/

using PtrVariant = std::variant<
	std::int8_t  *, std::int16_t  *, std::int32_t  *, std::int64_t  *,
	std::uint8_t *, std::uint16_t *, std::uint32_t *,
	bool *, float *, double *, std::string *
>;

template<typename V, typename Variant>
concept SettingVisitor = []<std::size_t... I>(std::index_sequence<I...>) {
	return (std::invocable<V, std::variant_alternative_t<I, Variant>> && ...);
} (std::make_index_sequence<std::variant_size_v<Variant>>{});

static_assert(SettingVisitor<decltype([](auto*) {}), PtrVariant>, "Visitor type mismatch!");

/*==================================================================*/

class SettingWrapper {
	PtrVariant mSettingPtr;

public:
	template <typename T>
	SettingWrapper(T* ptr) noexcept
		: mSettingPtr{ ptr }
	{}

	template <typename T>
	void set(T value) {
		std::visit([&value](auto* ptr) {
			using PtrT = std::decay_t<decltype(*ptr)>;
			if constexpr (std::is_assignable_v<PtrT&, T>)
				{ *ptr = std::forward<T>(value); }
		}, mSettingPtr);
	}

	template <typename T>
	T get(T value = T{}) const {
		return std::visit([&value](auto* ptr) -> T {
			using PtrT = std::decay_t<decltype(*ptr)>;
			if constexpr (std::is_convertible_v<PtrT, T>)
				{ return static_cast<T>(*ptr); }
			else
				{ return value; }
		}, mSettingPtr);
	}

	void visit(SettingVisitor<PtrVariant> auto&& visitor) {
		std::visit(std::forward<decltype(visitor)>(visitor), mSettingPtr);
	}

	void visit(SettingVisitor<PtrVariant> auto&& visitor) const {
		std::visit(std::forward<decltype(visitor)>(visitor), mSettingPtr);
	}
};

/*==================================================================*/

using SettingsMap = std::unordered_map<Str, SettingWrapper>;

template <typename T>
inline auto makeSetting(const Str& key, T* ptr) noexcept {
	return std::pair{ key, SettingWrapper{ ptr } };
}
