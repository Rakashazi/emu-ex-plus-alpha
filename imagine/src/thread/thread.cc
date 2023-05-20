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

#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#ifdef __linux__
#include <sched.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <cstring>

namespace IG
{

void setThreadCPUAffinityMask(std::span<const ThreadId> ids, CPUMask mask)
{
	#ifdef __linux__
	cpu_set_t cpuSet{};
	if(mask)
	{
		memcpy(&cpuSet, &mask, sizeof(mask));
	}
	else
	{
		memset(&cpuSet, 0xFF, sizeof(cpuSet));
	}
	for(auto id : ids)
	{
		// using direct syscall for compatibility with old Android versions
		if(syscall(__NR_sched_setaffinity, id, sizeof(cpuSet), &cpuSet) && Config::DEBUG_BUILD)
			logErr("error:%s setting thread:0x%X CPU affinity", strerror(errno), (unsigned)id);
	}
	#endif
}

void setThreadPriority(ThreadId id, int nice)
{
	#ifdef __linux__
	if(setpriority(PRIO_PROCESS, id, nice) && Config::DEBUG_BUILD)
		logErr("error:%s setting thread:0x%X nice level:%d", strerror(errno), (unsigned)id, nice);
	#endif
}

void setThisThreadPriority(int nice)
{
	#ifdef __linux__
	setThreadPriority(0, nice);
	#endif
}

int thisThreadPriority()
{
	#ifdef __linux__
	return getpriority(PRIO_PROCESS, 0);
	#else
	return 0;
	#endif
}

ThreadId thisThreadId()
{
	#ifdef __linux__
	return gettid();
	#else
	uint64_t id{};
	pthread_threadid_np(nullptr, &id);
	return id;
	#endif
}

}
