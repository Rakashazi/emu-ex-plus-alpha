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
#include <imagine/util/utility.h>
#include <concepts>
#include <thread>
#include <span>

namespace IG
{

static std::thread makeThreadSync(std::invocable<std::binary_semaphore&> auto &&f)
{
	std::binary_semaphore sem{0};
	if constexpr(std::is_copy_constructible_v<decltype(f)>)
	{
		std::thread t
		{
			[f{f}, &sem]()
			{
				f(sem);
			}
		};
		sem.acquire();
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
		sem.acquire();
		return t;
	}
}

static void makeDetachedThread(std::invocable auto &&f)
{
	std::thread t{IG_forward(f)};
	t.detach();
}

static void makeDetachedThreadSync(std::invocable<std::binary_semaphore&> auto &&f)
{
	std::binary_semaphore sem{0};
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
	sem.acquire();
}

#ifdef __linux__
using ThreadId = pid_t;
#else
using ThreadId = uint64_t;
#endif

using CPUMask = uint32_t;
static constexpr int maxCPUs = 32;

void setThreadCPUAffinityMask(std::span<const ThreadId>, CPUMask mask);
void setThreadPriority(ThreadId, int nice);
void setThisThreadPriority(int nice);
int thisThreadPriority();
ThreadId thisThreadId();

}
