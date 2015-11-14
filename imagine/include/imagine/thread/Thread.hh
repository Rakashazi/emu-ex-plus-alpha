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

#ifdef _WIN32
#include <imagine/thread/Win32Thread.hh>
#else
#include <imagine/thread/PThread.hh>
#endif

namespace IG
{

// Thread creation defined in implementation header:
// template<class F>
// void runOnThread(F func);

class Mutex : public MutexImpl
{
public:
	Mutex();
	~Mutex();
	void lock();
	void unlock();
};

class ConditionVar : public ConditionVarImpl
{
public:
	ConditionVar();
	~ConditionVar();
	void wait(Mutex &mutex);
	void notify_one();
};

}
