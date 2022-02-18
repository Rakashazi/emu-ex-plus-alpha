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

#include <imagine/util/concepts.hh>
#include <imagine/util/utility.h>
#include <thread>
#include <atomic>

namespace IG
{

class ThreadStop
{
public:
	void requestStop() { stopRequest.store(true, std::memory_order::relaxed); }
	void reset() { stopRequest.store(false, std::memory_order::relaxed); }
	operator bool() const { return stopRequest.load(std::memory_order::relaxed); }

protected:
	std::atomic_bool stopRequest{};
};

class WorkThread
{
public:
	WorkThread() {}

	WorkThread(auto &&f, auto &&...args)
		requires IG::invocable<decltype(f), ThreadStop &, decltype(args)...>:
		running{true},
		thread{makeThread(IG_forward(f), IG_forward(args)...)} {}

	~WorkThread() { stop(); }

	[[nodiscard]]
	bool joinable() const { return thread.joinable(); }
	void join() { thread.join(); }
	void requestStop() { threadStop.requestStop(); }
	[[nodiscard]]
	bool isRunning() const { return running.load(std::memory_order::relaxed); }

	void reset(auto &&f, auto &&...args)
		requires IG::invocable<decltype(f), ThreadStop &, decltype(args)...>
	{
		stop();
		threadStop.reset();
		running.store(true, std::memory_order::relaxed);
		thread = makeThread(IG_forward(f), IG_forward(args)...);
	}

	bool stop()
	{
		if(joinable())
		{
			requestStop();
			join();
			return true;
		}
		return false;
	}

protected:
	ThreadStop threadStop{};
	std::atomic_bool running{};
	std::thread thread;

	auto makeThread(auto &&f, auto &&...args)
	{
		return std::thread{[this, f = IG_forward(f)](auto &&...args)
		{
			f(threadStop, IG_forward(args)...);
			running.store(false, std::memory_order::relaxed);
		}, IG_forward(args)...};
	}
};

}
