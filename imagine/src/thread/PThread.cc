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
#include <imagine/thread/Thread.hh>
#include <assert.h>

namespace IG
{

Mutex::Mutex() {}

Mutex::~Mutex()
{
	pthread_mutex_destroy(&mutex);
}

void Mutex::lock()
{
	if(pthread_mutex_lock(&mutex) < 0)
	{
		logErr("error in pthread_mutex_lock");
	}
}

void Mutex::unlock()
{
	if(pthread_mutex_unlock(&mutex) < 0)
	{
		logErr("error in pthread_mutex_unlock");
	}
}

ConditionVar::ConditionVar() {}

ConditionVar::~ConditionVar()
{
	pthread_cond_destroy(&cond);
}

void ConditionVar::wait(Mutex &mutex)
{
	pthread_cond_wait(&cond, &mutex.nativeObject());
}

void ConditionVar::notify_one()
{
	pthread_cond_signal(&cond);
}

}
