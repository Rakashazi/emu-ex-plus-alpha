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

#include <engine-globals.h>
#include <logger/interface.h>
#include <util/DelegateFunc.hh>
#include <assert.h>
#include <pthread.h>
#ifndef __PPU__
#include <signal.h>
#endif

#if defined __ANDROID__ && CONFIG_ENV_ANDROID_MINSDK < 9
#include <base/android/private.hh>
#endif

class ThreadPThread
{
private:
	pthread_t id = 0;

	#if defined __ANDROID__ && CONFIG_ENV_ANDROID_MINSDK < 9
public:
	JNIEnv* jEnv = nullptr;
	#endif

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
		#ifndef __PPU__
		logMsg("sending signal %d to pthread %p", sig, (void*)id);
		pthread_kill(id, sig);
		#else
		bug_exit("signals not supported");
		#endif
	}

private:
	static void *wrapper(void *runData)
	{
		auto &run = *((ThreadPThread*)runData);
		//logMsg("running thread func %p", run->entry);
		#if defined __ANDROID__ && CONFIG_ENV_ANDROID_MINSDK < 9
		if(Base::jVM->AttachCurrentThread(&run.jEnv, 0) != 0)
		{
			logErr("error attaching jEnv to thread");
			return 0;
		}
		#endif
		auto res = run.entry(run);
		#if defined __ANDROID__ && CONFIG_ENV_ANDROID_MINSDK < 9
		if(run.jEnv)
			Base::jVM->DetachCurrentThread();
		#endif
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
	pthread_mutex_t *mutex = nullptr;
	bool init = false;

public:
	constexpr CondVarPThread() {}

	bool create(MutexPThread &mutex)
	{
		pthread_cond_init(&cond, 0);
		this->mutex = &mutex.mutex;
		init = true;
		return true;
	}

	void wait(MutexPThread *mutex = nullptr)
	{
		assert(mutex || this->mutex);
		pthread_mutex_t *waitMutex = mutex ? &mutex->mutex : this->mutex;
		pthread_cond_wait(&cond, waitMutex);
	}
};
