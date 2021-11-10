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
#include <new>
#include <cstddef>
#include <cassert>
#include <array>
#include <compare>

template <size_t, size_t, class, class ...> class DelegateFunc2;

template <size_t StorageSize, size_t Align, class R, class ...Args>
class DelegateFunc2<StorageSize, Align, R(Args...)>
{
public:
	using FreeFuncPtr = R (*)(Args...);

	constexpr DelegateFunc2() {}

	constexpr DelegateFunc2(std::nullptr_t) {}

	template<class F>
	requires IG::CallableClass<F, R, Args...> && (sizeof(F) <= StorageSize && Align >= std::alignment_of_v<F>)
	constexpr DelegateFunc2(F const &funcObj) :
		exec
		{
			[](const Storage &funcObj, Args ...args) -> R
			{
				return ((F*)funcObj.data())->operator()(std::forward<Args>(args)...);
			}
		}
	{
		// construct from lambda/function object
		new (store.data()) F(funcObj);
	}

	constexpr DelegateFunc2(IG::CallableFunctionPointer<R, Args...> auto const &funcObj)
		requires (sizeof(StorageSize) >= sizeof(void*) && Align >= sizeof(void*)):
		exec
		{
			[](const Storage &funcObj, Args ...args) -> R
			{
				return (*((FreeFuncPtr*)funcObj.data()))(std::forward<Args>(args)...);
			}
		}
	{
		// construct from free function
		new (store.data()) FreeFuncPtr(funcObj);
	}

	explicit constexpr operator bool() const
	{
		return exec;
	}

	constexpr R operator()(Args ...args) const
	{
		assert(exec);
		return exec(store, std::forward<Args>(args)...);
	}

	constexpr bool operator ==(DelegateFunc2 const&) const = default;

	constexpr R callCopy(Args ...args) const
	{
		// Call a copy to avoid trashing captured variables
		// if delegate's function can modify the delegate
		return ({auto copy = *this; copy;})(std::forward<Args>(args)...);
	}

	constexpr R callSafe(Args ...args) const
	{
		if(exec)
			return this->operator()(std::forward<Args>(args)...);
		return R();
	}

	constexpr R callCopySafe(Args ...args) const
	{
		if(exec)
			return callCopy(std::forward<Args>(args)...);
		return R();
	}

private:
	using Storage = std::array<unsigned char, StorageSize>;

	alignas(Align) Storage store{};
	R (*exec)(const Storage &, Args...){};
};

template <class R, class ...Args>
using DelegateFunc = DelegateFunc2<sizeof(void*)*2, sizeof(void*), R, Args...>;
