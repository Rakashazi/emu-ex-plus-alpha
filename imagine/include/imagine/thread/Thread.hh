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
#include <imagine/util/concepts.hh>
#include <thread>
#include <utility>
#include <pthread.h>

namespace IG
{

template<class T>
T thisThreadID()
{
	return static_cast<T>(pthread_self());
}

static std::thread makeThreadSync(IG::invocable<Semaphore&> auto &&f)
{
	Semaphore sem{0};
	if constexpr(std::is_copy_constructible_v<decltype(f)>)
	{
		std::thread t
		{
			[f{f}, &sem]()
			{
				f(sem);
			}
		};
		sem.wait();
		return t;
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
		sem.wait();
		return t;
	}
}

static void makeDetachedThread(IG::invocable auto &&f)
{
	std::thread t{std::forward<decltype(f)>(f)};
	t.detach();
}

static void makeDetachedThreadSync(IG::invocable<Semaphore&> auto &&f)
{
	Semaphore sem{0};
	if constexpr(std::is_copy_constructible_v<decltype(f)>)
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

void setThisThreadPriority(int nice);
int thisThreadPriority();

}
