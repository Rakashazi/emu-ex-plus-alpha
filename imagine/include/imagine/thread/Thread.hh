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

#include <imagine/config/defs.hh>
#include <imagine/thread/Semaphore.hh>
#include <thread>
#include <type_traits>
#include <utility>
#include <pthread.h>

namespace IG
{

template<class T>
T thisThreadID()
{
	return static_cast<T>(pthread_self());
}

template<class Func>
static void makeDetachedThread(Func &&f)
{
	std::thread t{std::forward<Func>(f)};
	t.detach();
}

template<class Func>
static void makeDetachedThreadSync(Func &&f)
{
	Semaphore sem{0};
	if constexpr(std::is_copy_constructible_v<Func>)
	{
		std::thread t
		{
			[f{f}, &sem]()
			{
				f(sem);
			}
		};
		t.detach();
	}
	else
	{
		std::thread t
		{
			[f{std::move(f)}, &sem]() mutable
			{
				f(sem);
			}
		};
		t.detach();
	}
	sem.wait();
}

}
