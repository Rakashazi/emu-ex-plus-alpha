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
#include <imagine/base/timerDefs.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/util/FunctionTraits.hh>

#if defined __linux
#include <imagine/base/timer/TimerFD.hh>
#elif defined __APPLE__
#include <imagine/base/timer/CFTimer.hh>
#endif

namespace Base
{

struct Timer : public TimerImpl
{
public:
	struct NullInit{};

	using TimerImpl::TimerImpl;
	explicit constexpr Timer(NullInit) {}
	Timer() : Timer{CallbackDelegate{}} {}
	Timer(const char *debugLabel): Timer{debugLabel, CallbackDelegate{}} {}
	void run(Time time, Time repeatTime, bool isAbsoluteTime = false, EventLoop loop = {}, CallbackDelegate c = {});
	void cancel();
	void setCallback(CallbackDelegate c);
	void dispatchEarly();
	bool isArmed();
	explicit operator bool() const;

	template<class Func>
	Timer(Func &&c):
		Timer{wrapCallbackDelegate(std::forward<Func>(c))}
	{}

	template<class Func>
	Timer(const char *debugLabel, Func &&c):
		Timer{debugLabel, wrapCallbackDelegate(std::forward<Func>(c))}
	{}

	template<class Func>
	void setCallback(Func &&c)
	{
		setCallback(wrapCallbackDelegate(std::forward<Func>(c)));
	}

	template<class Time1, class Time2, class Func = CallbackDelegate>
	void runIn(Time1 time, Time2 repeatTime, EventLoop loop = {}, Func &&c = nullptr)
	{
		run(std::chrono::duration_cast<Time>(time),
			std::chrono::duration_cast<Time>(repeatTime), false,
			loop, wrapCallbackDelegate(std::forward<Func>(c)));
	}

	template<class Time1, class Time2, class Func = CallbackDelegate>
	void runAt(Time1 time, Time2 repeatTime, EventLoop loop = {}, Func &&c = nullptr)
	{
		run(std::chrono::duration_cast<Time>(time),
			std::chrono::duration_cast<Time>(repeatTime), true,
			loop, wrapCallbackDelegate(std::forward<Func>(c)));
	}

	// non-repeating timer
	template<class Time1, class Func = CallbackDelegate>
	void runIn(Time1 time, EventLoop loop = {}, Func &&c = nullptr)
	{
		run(std::chrono::duration_cast<Time>(time), Time{}, false,
			loop, wrapCallbackDelegate(std::forward<Func>(c)));
	}

	template<class Time1, class Func = CallbackDelegate>
	void runAt(Time1 time, EventLoop loop = {}, Func &&c = nullptr)
	{
		run(std::chrono::duration_cast<Time>(time), Time{}, true,
			loop, wrapCallbackDelegate(std::forward<Func>(c)));
	}

	template<class Func>
	static CallbackDelegate wrapCallbackDelegate(Func &&func)
	{
		if constexpr(std::is_null_pointer_v<Func>)
		{
			return {};
		}
		constexpr auto returnsBool = std::is_same_v<bool, IG::FunctionTraitsR<Func>>;
		if constexpr(returnsBool)
		{
			return func;
		}
		else
		{
			return
				[=]()
				{
					func();
					return false;
				};
		}
	}
};

}
