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

template <class T, T VALUE, class Tag = bool>
struct EmptyConstant
{
	// accept dummy assignment from any value
	constexpr EmptyConstant() {}
	template <class ...Args>
	constexpr EmptyConstant(Args&& ...) {}

	constexpr operator T() const { return VALUE; };
};

template <class T, class Tag = bool>
struct EmptyType
{
	// accept dummy assignment from any value
	constexpr EmptyType() {}
	template <class ...Args>
	constexpr EmptyType(Args&& ...) {}

	constexpr operator T() const { return {}; };
};

// selects either type T and an empty type that converts to T and returns VALUE,
// used in combination with [[no_unique_address]] and a unique Tag type to declare
// a class member that conditionally doesn't consume any space without using the C-preprocessor
template<bool CONDITION, class T, T VALUE, class Tag = bool>
using UseTypeIfOrConstant = std::conditional_t<CONDITION, T, EmptyConstant<T, VALUE, Tag>>;

#define IG_enableMemberIfOrConstant(c, t, v, name) \
	struct name ## EmptyTag{}; \
	[[no_unique_address]] IG::UseTypeIfOrConstant<(c), t, v, name ## EmptyTag> name

// same as above but always returns a default constructed value so class types can be used
template<bool CONDITION, class T, class Tag = bool>
using UseTypeIf = std::conditional_t<CONDITION, T, EmptyType<T, Tag>>;

#define IG_enableMemberIf(c, t, name) \
	struct name ## EmptyTag{}; \
	[[no_unique_address]] IG::UseTypeIf<(c), t, name ## EmptyTag> name

static auto &deref(IG::Pointer auto &obj) { return *obj; }

static auto &deref(IG::NotPointer auto &obj) { return obj; }

}

#define static_assertIsStandardLayout(type) static_assert(std::is_standard_layout_v<type>, #type " isn't standard-layout")
#define static_assertHasTrivialDestructor(type) static_assert(std::is_trivially_destructible_v<type>, #type " has non-trivial destructor")
