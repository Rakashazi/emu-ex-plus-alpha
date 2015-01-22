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
#include <CoreFoundation/CoreFoundation.h>

namespace Base
{

class CFTimer
{
public:
	CFRunLoopTimerRef timer = nullptr;
	CallbackDelegate callback;
	CFTimeInterval repeat = 0;
	bool reuseResources = false; // whether to keep the CFRunLoopTimer in run-loop after firing
	bool armed = false;

	constexpr CFTimer() {}
	void callbackInCFAbsoluteTime(CallbackDelegate callback, CFAbsoluteTime relTime, CFTimeInterval repeatInterval, bool shouldReuseResources);
	void deinit();

	explicit operator bool() const
	{
		return armed;
	}
};

using TimerImpl = CFTimer;

}
