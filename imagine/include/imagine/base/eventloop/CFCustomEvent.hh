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

#include <imagine/base/eventLoopDefs.hh>
#include <CoreFoundation/CoreFoundation.h>

namespace Base
{

class CFCustomEvent
{
protected:
	CFRunLoopTimerRef timer{};
	CFRunLoopRef loop{};
	CustomEventDelegate callback;
	bool cancelled = true;
	#ifndef NDEBUG
	const char *debugLabel{};
	#endif

	const char *label();

public:
	#ifdef NDEBUG
	CFCustomEvent();
	CFCustomEvent(const char *debugLabel): CFCustomEvent() {}
	#else
	CFCustomEvent() : CFCustomEvent{nullptr} {}
	CFCustomEvent(const char *debugLabel);
	#endif

	explicit operator bool() const
	{
		return timer;
	}
};

using CustomEventImpl = CFCustomEvent;

}
