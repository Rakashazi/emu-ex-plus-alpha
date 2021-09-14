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
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <array>
#include <compare>

template <size_t, class, class ...> class DelegateFunc2;

template <size_t STORAGE_SIZE, class R, class ...Args> class DelegateFunc2<STORAGE_SIZE, R(Args...)>
{
public:
	using FreeFuncPtr = R (*)(Args...);

	constexpr DelegateFunc2() {}

	constexpr DelegateFunc2(std::nullptr_t) {}

	template<class F> requires IG::CallableClass<F, R, Args...> && (sizeof(F) <= STORAGE_SIZE)
	constexpr DelegateFunc2(F const &funcObj) :
		exec
		{
			[](const Storage &funcObj, Args... arguments) -> R
			{
				return ((F*)funcObj.data())->operator()(arguments...);
			}
		}
	{
		// construct from lambda/function object
		new (store.data()) F(funcObj);
	}

	constexpr DelegateFunc2(IG::CallableFunctionPointer<R, Args...> auto const &funcObj) :
		exec
		{
			[](const Storage &funcObj, Args... arguments) -> R
			{
				return callStoredFreeFunc(funcObj, arguments...);
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

	constexpr R operator()(const Args &... args) const
	{
		assert(exec);
		return exec(store, args...);
	}

	constexpr bool operator ==(DelegateFunc2 const&) const = default;

	constexpr R callCopy(const Args &... args) const
	{
		// Call a copy to avoid trashing captured variables
		// if delegate's function can modify the delegate
		return ({auto copy = *this; copy;})(args...);
	}

	constexpr R callSafe(const Args &... args) const
	{
		if(exec)
			return this->operator()(args...);
		return R();
	}

	constexpr R callCopySafe(const Args &... args) const
	{
		if(exec)
			return callCopy(args...);
		return R();
	}

private:
	using Storage = std::array<unsigned char, STORAGE_SIZE>;
	static_assert(sizeof(STORAGE_SIZE) >= sizeof(uintptr_t), "Storage must be large enough for 1 pointer");

	alignas(8) Storage store{};
	R (*exec)(const Storage &, Args...){};

	static constexpr R callStoredFreeFunc(const Storage &s, const Args &... args)
	{
		return (*((FreeFuncPtr*)s.data()))(args...);
	}
};

template <class R, class ...Args>
using DelegateFunc = DelegateFunc2<sizeof(uintptr_t)*2, R, Args...>;
