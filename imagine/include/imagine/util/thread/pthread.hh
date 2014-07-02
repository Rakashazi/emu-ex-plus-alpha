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

#include <imagine/engine-globals.h>
#include <imagine/logger/logger.h>
#include <imagine/util/DelegateFunc.hh>
#include <assert.h>
#include <pthread.h>
#include <signal.h>

class ThreadPThread
{
private:
	pthread_t id = 0;

public:
	using EntryDelegate = DelegateFunc<ptrsize (ThreadPThread &thread)>;
	EntryDelegate entry;
	bool running = 0;

	constexpr ThreadPThread() {}

	bool create(uint type, EntryDelegate entry)
	{
		this->entry = entry;

		pthread_attr_t attr;
		pthread_attr_init(&attr);
		if(type == 1)
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		else
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

		if(pthread_create(&id, &attr, wrapper, this) != 0)
		{
			logErr("error in pthread create");
			return false;
		}
		logMsg("created wrapped pthread %p", (void*)id);
		running = true;
		return true;
	}

	bool create(uint type, void* (*entry)(void *), void *arg)
	{
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		if(type == 1)
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		else
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

		if(pthread_create(&id, &attr, entry, arg) != 0)
		{
			logErr("error in pthread create");
			return 0;
		}
		logMsg("created pthread %p", (void*)id);
		running = 1;
		return 1;
	}

	void join()
	{
		logMsg("joining pthread %p", (void*)id);
		pthread_join(id, 0);
	}

	void kill(int sig)
	{
		logMsg("sending signal %d to pthread %p", sig, (void*)id);
		pthread_kill(id, sig);
	}

private:
	static void *wrapper(void *runData)
	{
		auto &run = *((ThreadPThread*)runData);
		//logMsg("running thread func %p", run->entry);
		auto res = run.entry(run);
		run.running = 0;
		return (void*)res;
	}
};

class MutexPThread
{
private:
	bool init = false, locked = false;

public:
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // TODO use accessor

	constexpr MutexPThread() {}

	bool create()
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		//pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		if(pthread_mutex_init(&mutex, &attr) != 0 )
		{
			logErr("error in pthread mutex init");
			return 0;
		}
		init = 1;
		locked = 0;
		logMsg("init mutex %p", &mutex);
		return 1;
	}

	void destroy()
	{
		assert(!locked);
		if(init)
		{
			logMsg("destroy mutex %p", &mutex);
			pthread_mutex_destroy(&mutex);
			init = 0;
		}
	}

	bool lock()
	{
		assert(init);
		//logMsg("lock mutex %p", &mutex);
		if(pthread_mutex_lock(&mutex) < 0)
		{
			logErr("error in pthread_mutex_lock");
			return 0;
		}
		locked = 1;
		return 1;
	}

	bool unlock()
	{
		assert(init);
		//logMsg("unlock mutex %p", &mutex);
		if(pthread_mutex_unlock(&mutex) < 0)
		{
			logErr("error in pthread_mutex_unlock");
			return 0;
		}
		locked = 0;
		return 1;
	}
};

class CondVarPThread
{
private:
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	bool isInit = false;

public:
	constexpr CondVarPThread() {}

	bool init()
	{
		if(isInit)
			return true;
		if(pthread_cond_init(&cond, nullptr))
			return false;
		isInit = true;
		return true;
	}

	void deinit()
	{
		if(isInit)
		{
			pthread_cond_destroy(&cond);
			isInit = false;
		}
	}

	void wait(MutexPThread &mutex)
	{
		assert(isInit);
		pthread_cond_wait(&cond, &mutex.mutex);
	}

	void signal()
	{
		assert(isInit);
		pthread_cond_signal(&cond);
	}
};
