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

#include <imagine/base/Base.hh>
#include <imagine/base/EventLoopFileSource.hh>
#include <sys/epoll.h>

namespace Base
{

static int ePoll = -1;
#ifdef CONFIG_BASE_X11
extern void x11FDHandler();
#endif

void EventLoopFileSource::init(int fd, PollEventDelegate callback, uint events)
{
	logMsg("adding fd %d to epoll", fd);
	fd_ = fd;
	var_selfs(callback);
	struct epoll_event ev {0};
	ev.data.ptr = this;
	ev.events = events;
	assert(ePoll != -1);
	epoll_ctl(ePoll, EPOLL_CTL_ADD, fd, &ev);
}

#ifdef CONFIG_BASE_X11
void EventLoopFileSource::initX(int fd)
{
  logMsg("adding X fd %d to epoll", fd);
	init(fd,
		[](int fd, int events)
		{
			x11FDHandler();
			return 1;
		});
}
#endif

void EventLoopFileSource::setEvents(uint events)
{
	struct epoll_event ev {0};
	ev.data.ptr = this;
	ev.events = events;
	assert(ePoll != -1);
	epoll_ctl(ePoll, EPOLL_CTL_MOD, fd_, &ev);
}

int EventLoopFileSource::fd() const
{
	return fd_;
}

void EventLoopFileSource::deinit()
{
	logMsg("removing fd %d from epoll", fd_);
	epoll_ctl(ePoll, EPOLL_CTL_DEL, fd_, nullptr);
}

static int epollWaitWrapper(int epfd, struct epoll_event *events, int maxevents)
{
	#ifdef CONFIG_BASE_X11
	x11FDHandler();  // must check X before entering epoll since some events may be
										// in memory queue and won't trigger the FD
	#endif
	return epoll_wait(epfd, events, maxevents, -1);
}

void initMainEventLoop()
{
	ePoll = epoll_create(16);
}

void runMainEventLoop()
{
	logMsg("entering epoll event loop");
	for(;;)
	{
		struct epoll_event event[16];
		int events;
		while((events = epollWaitWrapper(ePoll, event, sizeofArray(event))) > 0)
		{
			//logMsg("%d events ready", events);
			iterateTimes(events, i)
			{
				auto &e = *((EventLoopFileSource*)event[i].data.ptr);
				e.callback(e.fd_, event[i].events);
			}
		}
		if(events == -1)
		{
			if(errno == EINTR)
			{
				logMsg("epoll_wait interrupted by signal");
				continue;
			}
			else
				bug_exit("epoll_wait failed with errno %d", errno);
		}
	}
}

}
