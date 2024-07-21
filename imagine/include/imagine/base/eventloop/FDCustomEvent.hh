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
#include <imagine/util/used.hh>
#include <imagine/util/utility.h>
#include <concepts>
#include <optional>

namespace IG
{

struct FDCustomEventDesc
{
	const char* debugLabel{};
	std::optional<EventLoop> eventLoop;
};

class FDCustomEvent
{
public:
	static constexpr PollEventDelegate wrapDelegate(std::invocable auto&& del)
	{
		return [=](int fd, int)
		{
			if(shouldPerformCallback(fd))
				del();
			return true;
		};
	}

	FDCustomEvent(FDEventSourceDesc, PollEventDelegate);
	FDCustomEvent(FDEventSourceDesc desc, std::invocable auto&& del):FDCustomEvent{desc, wrapDelegate(IG_forward(del))} {}

	void attach(EventLoop loop = {}) { fdSrc.attach(loop); }

	void setCallback(std::invocable auto&& del)
	{
		fdSrc.setCallback(wrapDelegate(IG_forward(del)));
	}

	const char* debugLabel() const { return fdSrc.debugLabel(); }

protected:
	FDEventSource fdSrc;

	static bool shouldPerformCallback(int fd);
};

using CustomEventImpl = FDCustomEvent;

}
