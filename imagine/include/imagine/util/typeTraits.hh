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

#include <imagine/util/concepts.hh>

namespace IG
{

template<class T> struct dependentFalse : std::false_type {};

template <class T>
inline constexpr bool dependentFalseValue = dependentFalse<T>::value;

template <class T, T VALUE, int Tag = 0>
struct ConstantType
{
	// accept dummy assignment from any value
	constexpr ConstantType() = default;
	constexpr ConstantType(auto && ...) {}

	constexpr operator T() const { return VALUE; };
};

template <class T, int Tag = 0>
struct UnusedType
{
	struct UnusedTypeTag;

	// accept dummy assignment from any value
	constexpr UnusedType() = default;
	constexpr UnusedType(auto && ...) {}

	constexpr operator T() const { return {}; };
	constexpr operator bool() const requires (!IG::same_as<T, bool>) { return false; };

	// can take address of object, but always returns nullptr
	constexpr T* operator &() const { return nullptr; };
};

template <class T>
concept Unused = requires {typename T::UnusedTypeTag;};

// selects either type T and an empty type that converts to T and returns VALUE,
// used in combination with [[no_unique_address]] and a unique Tag value to declare
// a class member that conditionally doesn't consume any space without using the C-preprocessor
template<bool CONDITION, class T, T VALUE, int Tag = 0>
using UseIfOrConstant = std::conditional_t<CONDITION, T, ConstantType<T, VALUE, Tag>>;

#define IG_UseMemberIfOrConstant(c, t, v, name) \
	[[no_unique_address]] IG::UseIfOrConstant<(c), t, v, __LINE__> name

// same as above but always returns a default constructed value so class types can be used
template<bool CONDITION, class T, int Tag = 0>
using UseIf = std::conditional_t<CONDITION, T, UnusedType<T, Tag>>;

#define IG_UseMemberIf(c, t, name) \
	[[no_unique_address]] IG::UseIf<(c), t, __LINE__> name

// test that a variable's type is used in UseIf and not the UnusedType case
static constexpr bool used(auto &) { return true; }

static constexpr bool used(Unused auto &) { return false; }

// invoke func if v's type is used in UseIf
static constexpr void doIfUsed(auto &v, auto &&func)
{
	func(v);
}

static constexpr void doIfUsed(Unused auto &v, auto &&) {}

// same as above, but invoke defaultFunc if v's type is unused
static constexpr auto doIfUsedOr(auto &v, auto &&func, auto &&defaultFunc)
{
	return func(v);
}

static constexpr auto doIfUsedOr(Unused auto &v, auto &&func, auto &&defaultFunc)
{
	return defaultFunc();
}

static constexpr auto &deref(IG::Pointer auto &obj) { return *obj; }

static constexpr auto &deref(IG::NotPointer auto &obj) { return obj; }

}

#define static_assertIsStandardLayout(type) static_assert(std::is_standard_layout_v<type>, #type " isn't standard-layout")
#define static_assertHasTrivialDestructor(type) static_assert(std::is_trivially_destructible_v<type>, #type " has non-trivial destructor")
