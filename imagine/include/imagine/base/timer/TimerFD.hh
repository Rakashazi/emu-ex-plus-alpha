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
#include <imagine/base/EventLoop.hh>
#include <time.h>

namespace Base
{

class TimerFD
{
protected:
	FDEventSource fdSrc{};
	CallbackDelegate callback{};
	bool reuseResources = false; // whether to keep the timerfd open after firing
	bool repeating = false;
	bool armed = false;
	#ifndef NDEBUG
	const char *debugLabel{};
	#endif

	int fd() const;
	bool arm(timespec ms, timespec repeatInterval, EventLoop loop, bool shouldReuseResources);
	const char *label();

public:
	#ifdef NDEBUG
	TimerFD();
	TimerFD(const char *debugLabel): TimerFD() {}
	#else
	TimerFD() : TimerFD{nullptr} {}
	TimerFD(const char *debugLabel);
	#endif
	void deinit();
	void timerFired();

	bool operator ==(TimerFD const& rhs) const
	{
		return fdSrc.fd() == rhs.fdSrc.fd();
	}

	explicit operator bool() const
	{
		return armed;
	}
};

using TimerImpl = TimerFD;

}
