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
#include <imagine/util/concepts.hh>
#include <imagine/util/utility.h>

namespace IG
{

class FDCustomEvent
{
public:
	struct NullInit{};

	explicit constexpr FDCustomEvent(NullInit) {}
	FDCustomEvent() : FDCustomEvent{nullptr} {}
	FDCustomEvent(const char *debugLabel);

	FDCustomEvent(const char *debugLabel, EventLoop loop):
		FDCustomEvent{debugLabel}
	{
		attach(loop);
	}

	void attach(EventLoop loop, PollEventDelegate del);

	static constexpr PollEventDelegate makeDelegate(IG::invocable auto &&f)
	{
		return [=](int fd, int)
		{
			if(shouldPerformCallback(fd))
				f();
			return true;
		};
	}

	void attach(EventLoop loop) { attach(loop, makeDelegate([](){})); }

	void attach(auto &&f) { attach({}, IG_forward(f)); }

	void attach(EventLoop loop, IG::invocable auto &&f)	{ attach(loop, makeDelegate(IG_forward(f))); }

	void setCallback(IG::invocable auto &&f)
	{
		fdSrc.setCallback(makeDelegate(IG_forward(f)));
	}

protected:
	IG_UseMemberIf(Config::DEBUG_BUILD, const char *, debugLabel){};
	FDEventSource fdSrc{};

	const char *label();
	static bool shouldPerformCallback(int fd);
};

using CustomEventImpl = FDCustomEvent;

}
