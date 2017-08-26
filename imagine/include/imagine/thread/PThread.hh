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
#include <imagine/util/utility.h>
#include <pthread.h>

namespace IG
{

template<class F>
static pthread_t makePThread(F func)
{
	int res;
	pthread_t id;
	if(sizeof(F) <= sizeof(void*))
	{
		// inline function object data into the void* parameter
		union FuncData
		{
			F func;
			void *voidPtr;
		};
		FuncData funcData{func};
		res = pthread_create(&id, nullptr,
			[](void *arg) -> void*
			{
				((FuncData*)(&arg))->func();
				return nullptr;
			}, funcData.voidPtr);
	}
	else
	{
		void *funcPtr = new F(func);
		res = pthread_create(&id, nullptr,
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
		bug_unreachable("error in pthread create");
	}
	return id;
}

class PThread
{
protected:
	pthread_t id_{};

public:
	constexpr PThread() {}
	template<class Func>
	explicit PThread(Func&& f)
	{
		id_ = makePThread(f);
	}
};

using ThreadIDImpl = pthread_t;
using ThreadImpl = PThread;

}
