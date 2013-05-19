#pragma once
#include <engine-globals.h>
#include <logger/interface.h>
#include <util/DelegateFunc.hh>
#include <assert.h>
#include <pthread.h>
#ifndef CONFIG_BASE_PS3
	#include <signal.h>
#endif

#if defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK < 9
	#include <base/android/private.hh>
#endif

class ThreadPThread
{
public:
	bool running = 0;

	constexpr ThreadPThread() { }

	typedef DelegateFunc<ptrsize (ThreadPThread &thread)> EntryDelegate;

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
			return 0;
		}
		logMsg("created wrapped pthread %p", (void*)id);
		running = 1;
		return 1;
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
		#ifndef CONFIG_BASE_PS3
			logMsg("sending signal %d to pthread %p", sig, (void*)id);
			pthread_kill(id, sig);
		#else
			bug_exit("signals not supported");
		#endif
	}

	/*void cancel()
	{
		#ifndef CONFIG_BASE_PS3
		if(running)
		{
			logMsg("canceling pthread %d", (int)id);
			pthread_cancel(id);
			running = 0;
		}
		#else
			bug_exit("canceling thread not supported");
		#endif
	}*/

	EntryDelegate entry;
private:
	pthread_t id = 0;

#if defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK < 9
public:
	JNIEnv* jEnv = nullptr;
#endif

private:
	static void *wrapper(void *runData)
	{
		auto &run = *((ThreadPThread*)runData);
		//logMsg("running thread func %p", run->entry);
		#if defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK < 9
		if(Base::jVM->AttachCurrentThread(&run.jEnv, 0) != 0)
		{
			logErr("error attaching jEnv to thread");
			return 0;
		}
		#endif
		auto res = run.entry(run);
		#if defined(CONFIG_BASE_ANDROID) && CONFIG_ENV_ANDROID_MINSDK < 9
		if(run.jEnv)
			Base::jVM->DetachCurrentThread();
		#endif
		run.running = 0;
		return (void*)res;
	}
};

class MutexPThread
{
	bool init = 0, locked = 0;
public:
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // TODO use accessor
	constexpr MutexPThread() { }
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
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t *mutex = nullptr;
	bool init = 0;
public:
	constexpr CondVarPThread() { }
	bool create(MutexPThread *mutex = 0)
	{
		pthread_cond_init(&cond, 0);
		this->mutex = &mutex->mutex;
		init = 1;
		return 1;
	}

	void wait(MutexPThread *mutex = 0)
	{
		assert(mutex || this->mutex);
		pthread_mutex_t *waitMutex = mutex ? &mutex->mutex : this->mutex;
		pthread_cond_wait(&cond, waitMutex);
	}
};
