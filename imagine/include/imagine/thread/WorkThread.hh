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

#include <imagine/util/utility.h>
#include <concepts>
#include <thread>
#include <atomic>

namespace IG
{

class ThreadStop
{
public:
	static constexpr int8_t INTERRUPT = 1;
	static constexpr int8_t QUIT = 2;

	void requestStop(int8_t code = INTERRUPT) { stopRequest.store(code, std::memory_order::relaxed); }
	void reset() { stopRequest.store(0, std::memory_order::relaxed); }
	int8_t code() const { return stopRequest.load(std::memory_order::relaxed); }
	bool isQuitting() const { return code() == QUIT; }
	operator bool() const { return code(); }

protected:
	std::atomic_int8_t stopRequest{};
};

class WorkThread
{
public:
	struct Context
	{
		ThreadStop &stop;
		std::atomic_bool &working;

		void finishedWork() { working.store(false, std::memory_order::relaxed); }
	};

	WorkThread() {}

	WorkThread(auto &&f, auto &&...args)
		requires std::invocable<decltype(f), Context, decltype(args)...>:
		working{true},
		thread{makeThread(IG_forward(f), IG_forward(args)...)} {}

	~WorkThread() { stop(ThreadStop::QUIT); }

	[[nodiscard]]
	bool joinable() const { return thread.joinable(); }
	void join() { thread.join(); }
	void requestStop(int8_t code = ThreadStop::INTERRUPT) { threadStop.requestStop(code); }
	[[nodiscard]]
	bool isWorking() const { return working.load(std::memory_order::relaxed); }

	void reset(auto &&f, auto &&...args)
		requires std::invocable<decltype(f), Context, decltype(args)...>
	{
		stop(ThreadStop::QUIT);
		threadStop.reset();
		working.store(true, std::memory_order::relaxed);
		thread = makeThread(IG_forward(f), IG_forward(args)...);
	}

	bool stop(int8_t code = ThreadStop::INTERRUPT)
	{
		if(joinable())
		{
			requestStop(code);
			join();
			return true;
		}
		return false;
	}

protected:
	ThreadStop threadStop{};
	std::atomic_bool working{};
	std::thread thread;

	auto makeThread(auto &&f, auto &&...args)
	{
		return std::thread{[this, f = IG_forward(f)](auto &&...args)
		{
			f(Context{threadStop, working}, IG_forward(args)...);
			working.store(false, std::memory_order::relaxed);
		}, IG_forward(args)...};
	}
};

}
