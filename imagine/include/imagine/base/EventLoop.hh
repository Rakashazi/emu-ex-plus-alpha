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

#if defined CONFIG_BASE_GLIB
#include <imagine/base/eventloop/GlibEventLoop.hh>
#elif defined __ANDROID__
#include <imagine/base/eventloop/ALooperEventLoop.hh>
#elif defined __APPLE__
#include <imagine/base/eventloop/CFEventLoop.hh>
#endif

#include <imagine/base/eventLoopDefs.hh>

namespace Base
{

class EventLoop : public EventLoopImpl
{
public:
	using EventLoopImpl::EventLoopImpl;

	constexpr EventLoop() {}
	static EventLoop forThread();
	static EventLoop makeForThread();
	void run();
	void stop();
	explicit operator bool() const;

	template <class T>
	void run(const T &condition)
	{
		while((bool)condition)
		{
			run();
		}
	}
};

struct FDEventSource : public FDEventSourceImpl
{
public:
	using FDEventSourceImpl::FDEventSourceImpl;
	FDEventSource(int fd, EventLoop loop, PollEventDelegate callback, uint32_t events = POLLEV_IN):
		FDEventSource(nullptr, fd, loop, callback, events) {}
	FDEventSource(const char *debugLabel, int fd, EventLoop loop, PollEventDelegate callback, uint32_t events = POLLEV_IN);
	bool attach(PollEventDelegate callback, uint32_t events = POLLEV_IN);
	bool attach(EventLoop loop, PollEventDelegate callback, uint32_t events = POLLEV_IN);
	#if defined CONFIG_BASE_GLIB
	bool attach(EventLoop, GSource *, uint32_t events = POLLEV_IN);
	#endif
	void detach();
	void setEvents(uint32_t events);
	void dispatchEvents(uint32_t events);
	void setCallback(PollEventDelegate callback);
	bool hasEventLoop() const;
	int fd() const;
	void closeFD();
};

}
