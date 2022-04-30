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

#include <mach/semaphore.h>
#include <mach/task.h>
#include <mach/mach.h>
#include <utility>
#include <cassert>

// TODO: remove when <semaphore> is fully supported by Apple Clang

namespace IG
{

template<unsigned LeastMaxValue>
class MachSemaphore
{
public:
	MachSemaphore(unsigned startValue)
	{
		auto ret = semaphore_create(mach_task_self(), &sem, SYNC_POLICY_FIFO, startValue);
		assert(ret == KERN_SUCCESS);
	}

	MachSemaphore(MachSemaphore &&o) noexcept
	{
		*this = std::move(o);
	}

	MachSemaphore &operator=(MachSemaphore &&o) noexcept
	{
		deinit();
		sem = std::exchange(o.sem, {});
		return *this;
	}

	~MachSemaphore()
	{
		deinit();
	}

	void acquire()
	{
		semaphore_wait(sem);
	}

	void release()
	{
		semaphore_signal(sem);
	}

protected:
	semaphore_t sem{};

	void deinit()
	{
		if(!sem)
			return;
		auto ret = semaphore_destroy(mach_task_self(), sem);
		assert(ret == KERN_SUCCESS);
		sem = {};
	}
};

template<unsigned LeastMaxValue>
using SemaphoreImpl = MachSemaphore<LeastMaxValue>;

}

namespace std
{
template<unsigned LeastMaxValue>
using counting_semaphore = IG::SemaphoreImpl<LeastMaxValue>;

using binary_semaphore = std::counting_semaphore<1>;
}
