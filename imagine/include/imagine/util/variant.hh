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

#include <imagine/util/utility.h>
#include <variant>
#include <type_traits>

namespace IG
{

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts&&...) -> overloaded<std::decay_t<Ts>...>;

// Use a switch statement to implement visit() for better code generation,
// TODO: remove when clang implements a similar optimization,
// currently Clang 14's libc++ uses a function jump table that prevents inline optimization
template<class... VTypes, class Variant = std::variant<VTypes...>, auto vSize = std::variant_size_v<Variant>>
constexpr decltype(auto) visitVariant(auto&& func, Variant& v)
	requires (vSize <= 16)
{
	#define VISIT_CASE(i) case i: \
		if constexpr(i < vSize) { return func(*std::get_if<i>(&v)); } \
		else { __builtin_unreachable(); }
	switch(v.index())
	{
		VISIT_CASE(0);
		VISIT_CASE(1);
		VISIT_CASE(2);
		VISIT_CASE(3);
		VISIT_CASE(4);
		VISIT_CASE(5);
		VISIT_CASE(6);
		VISIT_CASE(7);
		VISIT_CASE(8);
		VISIT_CASE(9);
		VISIT_CASE(10);
		VISIT_CASE(11);
		VISIT_CASE(12);
		VISIT_CASE(13);
		VISIT_CASE(14);
		VISIT_CASE(15);
		VISIT_CASE(16);
	}
	#undef VISIT_CASE
	__builtin_unreachable();
}

template <class... VTypes>
constexpr decltype(auto) asVariant(std::variant<VTypes...>& v) noexcept { return v; }

template <class... VTypes>
constexpr decltype(auto) asVariant(const std::variant<VTypes...>& v) noexcept { return v; }

template <class... VTypes>
constexpr decltype(auto) asVariant(std::variant<VTypes...>&& v) noexcept { return std::move(v); }

template <class... VTypes>
constexpr decltype(auto) asVariant(const std::variant<VTypes...>&& v) noexcept { return std::move(v); }

// visit a std::variant or object derived from std::variant
template <class Func>
constexpr decltype(auto) visit(Func&& func, auto& v)
{
  return visitVariant(std::forward<Func>(func), asVariant(v));
}

class AddVisit
{
public:
	template <class Func>
	constexpr decltype(auto) visit(this auto&& self, Func&& func)
	{
	  return IG::visit(std::forward<Func>(func), asVariant(self));
	}

	bool operator==(AddVisit const&) const = default;
};

template<class T, class... VTypes>
constexpr T& getAs(std::variant<VTypes...>& v)
{
	auto vPtr = std::get_if<T>(&v);
	assumeExpr(vPtr);
	return *vPtr;
}

template<class T, class... VTypes>
constexpr const T& getAs(const std::variant<VTypes...>& v)
{
	auto vPtr = std::get_if<T>(&v);
	assumeExpr(vPtr);
	return *vPtr;
}

}
