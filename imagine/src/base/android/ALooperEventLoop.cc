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

#include <imagine/base/EventLoopFileSource.hh>
#include <imagine/logger/logger.h>

namespace Base
{

static ALooper *aLooper{};

static int pollEventCallback(int fd, int events, void *data)
{
	auto &source = *((EventLoopFileSource*)data);
	source.callback(fd, events);
	return 1;
}

ALooper *activityLooper()
{
	assert(aLooper);
	return aLooper;
}

void initActivityLooper()
{
	if(aLooper)
	{
		bug_exit("called setupActivityLooper() more than once");
	}
	aLooper = ALooper_forThread();
	assert(aLooper);
}

void EventLoopFileSource::init(int fd, PollEventDelegate callback, uint events)
{
	logMsg("adding fd %d to looper", fd);
	fd_ = fd;
	var_selfs(callback);
	assert(aLooper);
	int ret = ALooper_addFd(aLooper, fd, ALOOPER_POLL_CALLBACK, events, pollEventCallback, this);
	assert(ret == 1);
}

void EventLoopFileSource::setEvents(uint events)
{
	int ret = ALooper_addFd(aLooper, fd_, ALOOPER_POLL_CALLBACK, events, pollEventCallback, this);
	assert(ret == 1);
}

int EventLoopFileSource::fd() const
{
	return fd_;
}

void EventLoopFileSource::deinit()
{
	logMsg("removing fd %d from looper", fd_);
	int ret = ALooper_removeFd(aLooper, fd_);
	assert(ret != -1);
}

}
