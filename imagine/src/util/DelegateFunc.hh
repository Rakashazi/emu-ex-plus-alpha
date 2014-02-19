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

#include <assert.h>
#include <new>
#include <type_traits>
#include <cstdint>

template <typename T> class DelegateFunc {};

template <typename R, typename ...ARGS> class DelegateFunc<R(ARGS...)>
{
public:
	static constexpr int STORAGE_SIZE = sizeof(uintptr_t)*2;

	constexpr DelegateFunc() {}

	// construct from lambda
	template<class T>
	DelegateFunc(T const &lambda /*, typename std::enable_if<!std::is_function<T>::value>::type* = 0*/) :
		exec
		{
			[](const Storage &lambda, ARGS... arguments) -> R
			{
				return ((T*)lambda.data())->operator()(arguments...);
			}
		}
	{
		static_assert(sizeof(T) <= STORAGE_SIZE, "Delegate too big for storage");
		new (this->lambdaMem.mem) T(lambda);
	}

	// construct from free function
	/*template <class T>
	constexpr DelegateFunc(T const &func, typename std::enable_if<std::is_function<T>::value>::type* = 0) :
		exec
		{
			[](const Storage &lambda, ARGS... arguments) -> R
			{
				return lambda.func(arguments...);
			}
		},
		lambdaMem{func}
	{}*/

	operator bool() const
	{
		return exec;
	}

	R operator()(ARGS ... in) const
	{
		assert(exec);
		return exec(lambdaMem, in...);
	}

	bool operator ==(DelegateFunc const &rhs) const
	{
		return exec == rhs.exec;
	}

private:
	struct Storage
	{
		constexpr Storage() {}
		//constexpr Storage(R (*func)(ARGS...)): func{func} {}
		const void *data() const { return mem; }
		//union
		//{
			//R (*func)(ARGS...);
			char mem[STORAGE_SIZE] {0};
		//};
	};

	R (*exec)(const Storage &, ARGS...) = nullptr;
	Storage lambdaMem;
};
