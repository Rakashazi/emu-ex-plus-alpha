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
constexpr bool dependentFalseValue = dependentFalse<T>::value;

constexpr auto &deref(IG::Pointer auto &obj) { return *obj; }

constexpr auto &deref(IG::NotPointer auto &obj) { return obj; }

}

#define static_assertIsStandardLayout(type) static_assert(std::is_standard_layout_v<type>, #type " isn't standard-layout")
#define static_assertHasTrivialDestructor(type) static_assert(std::is_trivially_destructible_v<type>, #type " has non-trivial destructor")
