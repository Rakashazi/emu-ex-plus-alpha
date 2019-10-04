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

#define LOGTAG "CustomEvent"
#include <imagine/base/CustomEvent.hh>
#include <imagine/logger/logger.h>
#include <unistd.h>
#include <sys/eventfd.h>

namespace Base
{

void CustomEvent::setEventLoop(EventLoop loop)
{
	if(fd == -1)
	{
		fd = eventfd(0, 0);
		if(fd == -1)
		{
			logErr("error creating eventfd");
			return;
		}
	}
	if(!loop)
		loop = EventLoop::forThread();
	fdSrc = {fd, loop,
		[this](int fd, int events)
		{
			eventfd_t notify;
			auto ret = read(fd, &notify, sizeof(notify));
			assert(ret == sizeof(notify));
			if(cancelled)
				return true;
			callback.callSafe();
			return true;
		}};
}

void CustomEvent::setCallback(CustomEventDelegate c)
{
	callback = c;
}

void CustomEvent::notify()
{
	cancelled = false;
	eventfd_t notify = 1;
	auto ret = write(fd, &notify, sizeof(notify));
	assert(ret == sizeof(notify));
}

void CustomEvent::cancel()
{
	cancelled = true;
}

void CustomEvent::deinit()
{
	cancelled = true;
	fdSrc.removeFromEventLoop();
	close(fd);
	fd = -1;
}

}
