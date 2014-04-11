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

#include <imagine/engine-globals.h>
#include <imagine/base/eventLoopDefs.hh>

#if defined CONFIG_BASE_GLIB
#include <imagine/base/eventloop/GlibEventLoop.hh>
#elif defined __ANDROID__
#include <imagine/base/eventloop/ALooperEventLoop.hh>
#elif defined __linux
#include <imagine/base/eventloop/EPollEventLoop.hh>
#elif defined __APPLE__
#include <imagine/base/eventloop/CFEventLoop.hh>
#endif

namespace Base
{

struct EventLoopFileSource : public EventLoopFileSourceImpl
{
public:
	constexpr EventLoopFileSource() {}
	void init(int fd, PollEventDelegate callback, uint events);
	void init(int fd, PollEventDelegate callback)
	{
		init(fd, callback, POLLEV_IN);
	}
  #ifdef CONFIG_BASE_X11
	void initX(int fd);
  #endif
	void setEvents(uint events);
	int fd() const;
	void deinit();
};

}
