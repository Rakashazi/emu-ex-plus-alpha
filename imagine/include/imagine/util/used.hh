#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <concepts>

namespace IG
{

template <class T, T val, int tag = 0>
struct ConstantType
{
	using Type = T;
	struct UnusedTypeTag;

	// accept dummy assignment from any value
	constexpr ConstantType() = default;
	constexpr ConstantType(auto && ...) {}

	constexpr const T &value() const { return value_; }
	constexpr operator const T&() const { return value_; };

	constexpr auto& operator +=(const auto &) { return *this; }
	constexpr auto& operator -=(const auto &) { return *this; }
	constexpr auto& operator *=(const auto &) { return *this; }
	constexpr auto& operator /=(const auto &) { return *this; }
	constexpr auto operator<=>(const T &o) const { return value() <=> o; };

private:
	static constexpr T value_{val};
};

template <class T, int tag = 0>
struct UnusedType
{
	using Type = T;
	struct UnusedTypeTag;

	// accept dummy assignment from any value
	constexpr UnusedType() = default;
	constexpr UnusedType(auto && ...) {}

	constexpr T value() const { return {}; }
	constexpr operator T() const { return {}; };
	explicit constexpr operator bool() const requires (!std::same_as<T, bool>) { return false; };

	// can take address of object, but always returns nullptr
	constexpr T* operator &() const { return nullptr; };

	constexpr auto& operator +=(const auto &) { return *this; }
	constexpr auto& operator -=(const auto &) { return *this; }
	constexpr auto& operator *=(const auto &) { return *this; }
	constexpr auto& operator /=(const auto &) { return *this; }
	constexpr auto operator<=>(const T &o) const { return T{} <=> o; };
};

template <class T>
concept Unused = requires {typename T::UnusedTypeTag;};

// selects either type T and an empty type that converts to T and returns VALUE,
// used in combination with [[no_unique_address]] and a unique Tag value to declare
// a class member that conditionally doesn't consume any space without using the C-preprocessor
template<bool condition, class T, T value, int tag = 0>
using UseIfOrConstant = std::conditional_t<condition, T, ConstantType<T, value, tag>>;

template<int tag>
struct UseIfOrConstantTagInjector // used to inject the line count as "tag" when used from a macro
{
    template<bool condition, class T, T value>
    using T = UseIfOrConstant<condition, T, value, tag>;
};

#define ConditionalMemberOr [[no_unique_address]] IG::UseIfOrConstantTagInjector<__LINE__>::T

// same as above but always returns a default constructed value so class types can be used
template<bool condition, class T, int tag = 0>
using UseIf = std::conditional_t<condition, T, UnusedType<T, tag>>;

template<int tag>
struct UseIfTagInjector
{
    template<bool condition, class T>
    using T = UseIf<condition, T, tag>;
};

#define ConditionalMember [[no_unique_address]] IG::UseIfTagInjector<__LINE__>::T

// test that a variable's type is used in UseIf and not the UnusedType case
constexpr bool used(auto &&) { return true; }
constexpr bool used(auto &) { return true; }

constexpr bool used(Unused auto &&) { return false; }
constexpr bool used(Unused auto &) { return false; }

// invoke func if v's type doesn't satisfy the Unused concept
template<class R = int>
constexpr auto doIfUsed(auto& v, auto&& func, [[maybe_unused]] R &&defaultReturn = 0)
{
	return func(v);
}

template<class R = int>
constexpr auto doIfUsed(Unused auto&, auto&&, R &&defaultReturn = 0)
{
	return defaultReturn;
}

// same as above, but invoke defaultFunc if v's type is unused
constexpr auto doIfUsedOr(auto& v, auto&& func, auto&&)
{
	return func(v);
}

constexpr auto doIfUsedOr(Unused auto&, auto&&, auto &&defaultFunc)
{
	return defaultFunc();
}

#define IG_GetDefaultValueOr(value, orValue) \
[]() \
{ \
    if constexpr(requires {value;}) \
        return decltype(value)(); \
    else \
        return orValue; \
}()

#define IG_GetValueTypeOr(value, OrType) decltype(IG_GetDefaultValueOr(value, OrType()))

}
