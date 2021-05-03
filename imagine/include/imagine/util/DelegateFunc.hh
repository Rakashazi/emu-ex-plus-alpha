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

#include <new>
#include <cstdint>
#include <cstddef>
#include <array>
#include <compare>
#include <type_traits>
#include <imagine/util/utility.h>

template <size_t, typename, typename ...> class DelegateFunc2;

template <size_t STORAGE_SIZE, typename R, typename ...Args> class DelegateFunc2<STORAGE_SIZE, R(Args...)>
{
public:
	constexpr DelegateFunc2() {}

	constexpr DelegateFunc2(std::nullptr_t) {}

	template<class T>
	constexpr DelegateFunc2(T const &funcObj) :
		exec
		{
			[](const Storage &funcObj, Args... arguments) -> R
			{
				if constexpr(isCompatibleFreeFunc<T>())
					return callStoredFreeFunc(funcObj, arguments...);
				else
					return ((T*)funcObj.data())->operator()(arguments...);
			}
		}
	{
		if constexpr(isCompatibleFreeFunc<T>())
		{
			// construct from free function
			new (store.data()) FreeFuncPtr(funcObj);
		}
		else
		{
			// construct from lambda
			static_assert(sizeof(T) <= STORAGE_SIZE, "Delegate too big for storage");
			new (store.data()) T(funcObj);
		}
	}

	explicit constexpr operator bool() const
	{
		return exec;
	}

	constexpr R operator()(Args... args) const
	{
		assert(exec);
		return exec(store, args...);
	}

	constexpr bool operator ==(DelegateFunc2 const&) const = default;

	constexpr R callCopy(Args... args) const
	{
		// Call a copy to avoid trashing captured variables
		// if delegate's function can modify the delegate
		return IG::copySelf(*this)(args...);
	}

	constexpr R callSafe(Args... args) const
	{
		if(exec)
			return this->operator()(args...);
		return R();
	}

	constexpr R callCopySafe(Args... args) const
	{
		if(exec)
			return callCopy(args...);
		return R();
	}

	template<class T>
	static constexpr bool isCompatibleFreeFunc()
	{
		return std::is_convertible_v<T, FreeFuncPtr>;
	}

private:
	using FreeFuncPtr = R (*)(Args...);
	using Storage = std::array<unsigned char, STORAGE_SIZE>;
	static_assert(sizeof(STORAGE_SIZE) >= sizeof(uintptr_t), "Storage must be large enough for 1 pointer");

	alignas(8) Storage store{};
	R (*exec)(const Storage &, Args...){};

	static constexpr R callStoredFreeFunc(const Storage &s, Args... args)
	{
		return (*((FreeFuncPtr*)s.data()))(args...);
	}
};

template <typename R, typename ...Args>
using DelegateFunc = DelegateFunc2<sizeof(uintptr_t)*2, R, Args...>;
