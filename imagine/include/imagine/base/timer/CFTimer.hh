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

#include <imagine/base/baseDefs.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/used.hh>
#include <CoreFoundation/CoreFoundation.h>
#include <memory>

namespace IG
{

struct CFTimerInfo
{
	CallbackDelegate callback{};
	CFRunLoopRef loop{};
	CFRunLoopRef setLoop{};
};

class CFTimer
{
public:
	using TimePoint = SteadyClockTimePoint;

	CFTimer(TimerDesc, CallbackDelegate);
	CFTimer(CFTimer&&) noexcept;
	CFTimer &operator=(CFTimer&&) noexcept;
	~CFTimer();
	const char* debugLabel() const { return debugLabel_; }

protected:
	ConditionalMember<Config::DEBUG_BUILD, const char *> debugLabel_{};
	CFRunLoopTimerRef timer{};
	std::unique_ptr<CFTimerInfo> info;

	void callbackInCFAbsoluteTime(CFAbsoluteTime absTime, CFTimeInterval repeatInterval, CFRunLoopRef loop);
	void deinit();
};

using TimerImpl = CFTimer;

}
