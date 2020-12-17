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
#include <imagine/time/Time.hh>
#include <imagine/util/typeTraits.hh>
#include <CoreFoundation/CoreFoundation.h>
#include <memory>

namespace Base
{

struct CFTimerInfo
{
	CallbackDelegate callback{};
	CFRunLoopRef loop{};
};

class CFTimer
{
public:
	using Time = IG::FloatSeconds;

	constexpr CFTimer() {}
	CFTimer(CallbackDelegate c) : CFTimer{nullptr, c} {}
	CFTimer(const char *debugLabel, CallbackDelegate c);
	CFTimer(CFTimer &&o);
	CFTimer &operator=(CFTimer &&o);
	~CFTimer();

protected:
	IG_enableMemberIf(Config::DEBUG_BUILD, const char *, debugLabel){};
	CFRunLoopTimerRef timer{};
	std::unique_ptr<CFTimerInfo> info{};

	void callbackInCFAbsoluteTime(CFAbsoluteTime absTime, CFTimeInterval repeatInterval, CFRunLoopRef loop);
	void deinit();
	const char *label();
};

using TimerImpl = CFTimer;

}
