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
#include <pthread.h>

namespace IG
{

template<class F>
static void runOnThread(F func)
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	int res;
	if(sizeof(F) <= sizeof(void*))
	{
		// inline function object data into the void* parameter
		union FuncData
		{
			F func;
			void *voidPtr;
		};
		FuncData funcData{func};
		pthread_t id;
		res = pthread_create(&id, &attr,
			[](void *arg) -> void*
			{
				((FuncData*)(&arg))->func();
				return nullptr;
			}, funcData.voidPtr);
	}
	else
	{
		void *funcPtr = new F(func);
		pthread_t id;
		res = pthread_create(&id, &attr,
			[](void *arg) -> void*
			{
				auto funcPtr = (F*)arg;
				auto func = *funcPtr;
				delete funcPtr;
				func();
				return nullptr;
			}, funcPtr);
	}
	if(res != 0)
	{
		bug_exit("error in pthread create");
	}
}

class PThreadMutex
{
protected:
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

public:
	pthread_mutex_t nativeObject()
	{
		return mutex;
	}
};

using MutexImpl = PThreadMutex;

class PThreadConditionVar
{
protected:
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
};

using ConditionVarImpl = PThreadConditionVar;

}
