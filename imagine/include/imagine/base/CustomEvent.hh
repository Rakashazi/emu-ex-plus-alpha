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

#include <imagine/config/defs.hh>
#include <imagine/base/EventLoop.hh>

#if defined __linux
#include <imagine/base/eventloop/FDCustomEvent.hh>
#elif defined __APPLE__
#include <imagine/base/eventloop/CFCustomEvent.hh>
#endif

namespace Base
{

struct CustomEvent : public CustomEventImpl
{
public:
	CustomEvent() {}
	void setEventLoop(EventLoop loop);
	void setCallback(CustomEventDelegate callback);
	void notify();
	void cancel();
	void deinit();
};

}
