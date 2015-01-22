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

#include <imagine/base/timerDefs.hh>
#include <imagine/base/EventLoopFileSource.hh>
#include <time.h>

namespace Base
{

class TimerFD
{
protected:
	#if !defined __ANDROID__
	EventLoopFileSource fdSrc;
	#endif
	CallbackDelegate callback;
	int fd = -1;
	bool reuseResources = false; // whether to keep the timerfd open after firing
	bool repeating = false;
	bool armed = false;

	bool arm(timespec ms, timespec repeatInterval, bool shouldReuseResources);

public:
	constexpr TimerFD() {}
	void deinit();
	void timerFired();

	bool operator ==(TimerFD const& rhs) const
	{
		return fd == rhs.fd;
	}

	explicit operator bool() const
	{
		return armed;
	}
};

using TimerImpl = TimerFD;

}
