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
#include <cstring>
#include <type_traits>
#include <imagine/util/utility.h>

template <size_t, typename, typename ...> class DelegateFunc2;

template <size_t STORAGE_SIZE, typename R, typename ...ARGS> class DelegateFunc2<STORAGE_SIZE, R(ARGS...)>
{
public:
	constexpr DelegateFunc2() {}

	constexpr DelegateFunc2(std::nullptr_t) {}

	template<class T>
	constexpr DelegateFunc2(T const &funcObj) :
		exec
		{
			[](const Storage &funcObj, ARGS... arguments) -> R
			{
				if constexpr(isCompatibleFreeFunc<T>())
					return funcObj.func(arguments...);
				else
					return ((T*)funcObj.data())->operator()(arguments...);
			}
		}
	{
		if constexpr(isCompatibleFreeFunc<T>())
		{
			// construct from free function
			execData = Storage(funcObj);
		}
		else
		{
			// construct from lambda
			static_assert(sizeof(T) <= STORAGE_SIZE, "Delegate too big for storage");
			new (execData.mem) T(funcObj);
		}
	}

	explicit operator bool() const
	{
		return exec;
	}

	R operator()(ARGS ... in) const
	{
		assumeExpr(exec);
		return exec(execData, in...);
	}

	bool operator ==(DelegateFunc2 const &rhs) const
	{
		return std::memcmp(this, &rhs, sizeof(DelegateFunc2)) == 0;
	}

	R callCopy(ARGS ... in) const
	{
		// Call a copy to avoid trashing captured variables
		// if delegate's function can modify the delegate
		auto del = *this;
		return del(in...);
	}

	R callSafe(ARGS ... in) const
	{
		if(exec)
			return this->operator()(in...);
		return R();
	}

	R callCopySafe(ARGS ... in) const
	{
		if(exec)
			return callCopy(in...);
		return R();
	}

	template<class T>
	static constexpr bool isCompatibleFreeFunc()
	{
		return std::is_convertible_v<T, R (*)(ARGS...)>;
	}

private:
	struct Storage
	{
		constexpr Storage() {}
		constexpr Storage(R (*func)(ARGS...)): func{func} {}
		const void *data() const { return mem; }
		union
		{
			R (*func)(ARGS...){};
			char mem[STORAGE_SIZE];
		};
	};

	R (*exec)(const Storage &, ARGS...){};
	Storage execData;
};

template <typename R, typename ...ARGS>
using DelegateFunc = DelegateFunc2<sizeof(uintptr_t)*2, R, ARGS...>;
