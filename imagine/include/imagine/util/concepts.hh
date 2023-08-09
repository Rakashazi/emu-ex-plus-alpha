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
#include <type_traits>
#include <iterator>
#include <functional>

namespace IG
{

template <class T>
concept Arithmetic = std::is_arithmetic_v<T>;

template <class T>
concept Pointer = std::is_pointer_v<T>;

template <class T>
concept NotPointer = !Pointer<T>;

template <class T, size_t size>
concept PointerOfSize = Pointer<T> && sizeof(std::remove_pointer_t<T>) == size;

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
concept ResizableContainer =
	Container<T> &&
	requires(T &&c)
	{
		c.resize(1);
	};

template <class T>
concept Iterable =
	requires(T &&c)
	{
		std::begin(c);
		std::end(c);
	};

template<class F, class Return, class... Args>
concept SameInvokeResult = std::same_as<std::invoke_result_t<F, Args...>, Return>;

template<class F, class Return, class... Args>
concept ConvertibleInvokeResult = std::convertible_to<std::invoke_result_t<F, Args...>, Return>;

template<class F, class... Args>
concept VoidInvokeResult = std::same_as<std::invoke_result_t<F, Args...>, void>;

template<class F, class Return, class... Args>
concept Callable = std::invocable<F, Args...> && ConvertibleInvokeResult<F, Return, Args...>;

template<class F, class Return, class... Args>
concept CallableClass = Class<F> && Callable<F, Return, Args...>;

template<class F, class Return, class... Args>
concept CallableFunctionPointer = Pointer<F> && Callable<F, Return, Args...>;

template<class F, class... Args>
concept ValidInvokeArgs =
	requires(F f, Args &&...args)
	{
		 f(std::forward<Args>(args)...);
	};


template <class T>
concept Const = std::is_const_v<T>;

constexpr auto &indirect(Pointer auto &obj) { return *obj; }

constexpr auto &indirect(NotPointer auto &obj) { return obj; }

}
