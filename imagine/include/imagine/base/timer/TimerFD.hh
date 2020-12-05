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
#include <imagine/time/Time.hh>
#include <imagine/util/typeTraits.hh>
#include <time.h>
#include <memory>

namespace Base
{

class TimerFD
{
public:
	using Time = IG::Nanoseconds;

	constexpr TimerFD() {}
	TimerFD(CallbackDelegate c) : TimerFD{nullptr, c} {}
	TimerFD(const char *debugLabel, CallbackDelegate c);
	TimerFD(TimerFD &&o);
	TimerFD &operator=(TimerFD &&o);
	~TimerFD();

protected:
	IG_enableMemberIf(Config::DEBUG_BUILD, const char *, debugLabel){};
	std::unique_ptr<CallbackDelegate> callback_{};
	FDEventSource fdSrc{};

	void deinit();
	bool arm(timespec ms, timespec repeatInterval, EventLoop loop);
	const char *label();
};

using TimerImpl = TimerFD;

}
