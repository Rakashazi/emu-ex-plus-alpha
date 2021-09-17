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

#if __has_include(<concepts>)
#include <concepts>
#endif
#include <type_traits>
#include <iterator>
#include <functional>

namespace IG
{

#if !__has_include(<concepts>)
namespace detail
{
	template <class T, class U>
	concept SameHelper = std::is_same_v<T, U>;
}

template <class T, class U>
concept same_as = detail::SameHelper<T, U> && detail::SameHelper<U, T>;
#else
template <class T, class U>
concept same_as = std::same_as<T, U>;
#endif

// TODO: remove concepts with standard versions in <concepts> once fully supported by Clang
template <class T>
concept integral = std::is_integral_v<T>;

template <class T>
concept signed_integral = integral<T> && std::is_signed_v<T>;

template <class T>
concept unsigned_integral = integral<T> && !signed_integral<T>;

template <class T>
concept floating_point = std::is_floating_point_v<T>;

template<class Derived, class Base>
concept derived_from =
	std::is_base_of_v<Base, Derived> &&
	std::is_convertible_v<const volatile Derived*, const volatile Base*>;

template <class From, class To>
concept convertible_to =
	std::is_convertible_v<From, To> &&
	requires(std::add_rvalue_reference_t<From> (&f)()) {
		static_cast<To>(f());
	};

template<class F, class... Args>
concept invocable =
	requires(F&& f, Args&&... args)
	{
		std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
	};

template <class T>
concept Pointer = std::is_pointer_v<T>;

template <class T>
concept NotPointer = !Pointer<T>;

template <class T>
concept PointerDecayable = Pointer<std::decay_t<T>>;

template <class T>
concept NotPointerDecayable = !PointerDecayable<T>;

template <class T>
concept NullPointer = std::is_null_pointer_v<T>;

template <class T>
concept Class = std::is_class_v<T>;

template <class T>
concept Container =
	requires(T &&c)
	{
		std::size(c);
		std::data(c);
	};

template <class T>
concept Iterable =
	requires(T &&c)
	{
		std::begin(c);
		std::end(c);
	};

template<class F, class Return, class... Args>
concept SameInvokeResult = same_as<std::invoke_result_t<F, Args...>, Return>;

template<class F, class Return, class... Args>
concept ConvertibleInvokeResult = convertible_to<std::invoke_result_t<F, Args...>, Return>;

template<class F, class Return, class... Args>
concept Callable = invocable<F, Args...> && ConvertibleInvokeResult<F, Return, Args...>;

template<class F, class Return, class... Args>
concept CallableClass = Class<F> && Callable<F, Return, Args...>;

template<class F, class Return, class... Args>
concept CallableFunctionPointer = Pointer<F> && Callable<F, Return, Args...>;

template<class F, class... Args>
concept Predicate = invocable<F, Args...> && ConvertibleInvokeResult<F, bool, Args...>;

}
