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

#ifdef _WIN32
#include <imagine/thread/Win32Thread.hh>
#else
#include <imagine/thread/PThread.hh>
#endif

namespace IG
{

class thread : public ThreadImpl
{
public:
	using id = ThreadIDImpl;

	thread();
	~thread();
	thread(thread&& other);
	template<class Function>
	explicit thread(Function&& f) : ThreadImpl{f} {}
	template<class Function>
	explicit thread(Function&& f, bool detached) : ThreadImpl{f, detached} {}
	thread(const thread&) = delete;
	bool joinable() const;
	id get_id() const;
	void join();
	void detach();
};

namespace this_thread
{

thread::id get_id();

}

template<class Func>
static void makeDetachedThread(Func&& f)
{
	thread t{f, true};
}

template<class Func>
static void makeDetachedThreadSync(Func&& f)
{
	Semaphore sem{0};
	thread t{[=, &sem](){f(sem);}, true};
	sem.wait();
}

}
