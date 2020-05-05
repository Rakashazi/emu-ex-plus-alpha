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

#include <imagine/base/EventLoop.hh>
#include <utility>

namespace Base
{

class FDCustomEvent
{
public:
	#ifdef NDEBUG
	FDCustomEvent();
	FDCustomEvent(const char *debugLabel): FDCustomEvent() {}
	#else
	FDCustomEvent() : FDCustomEvent{nullptr} {}
	FDCustomEvent(const char *debugLabel);
	#endif
	FDCustomEvent(FDCustomEvent &&o);
	FDCustomEvent &operator=(FDCustomEvent &&o);
	~FDCustomEvent();
	void attach(EventLoop loop, PollEventDelegate del);

	template<class Func>
	void attach(Func func)
	{
		attach({}, std::forward<Func>(func));
	}

	template<class Func>
	void attach(EventLoop loop, Func func)
	{
		attach(loop,
			PollEventDelegate
			{
				[=](int fd, int)
				{
					if(shouldPerformCallback(fd))
						func();
					return true;
				}
			});
	}

protected:
	#ifndef NDEBUG
	const char *debugLabel{};
	#endif
	FDEventSource fdSrc{};

	const char *label();
	void deinit();
	static bool shouldPerformCallback(int fd);
};

using CustomEventImpl = FDCustomEvent;

}
