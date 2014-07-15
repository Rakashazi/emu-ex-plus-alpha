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
#include <glib-unix.h>

namespace Base
{

#ifdef CONFIG_BASE_X11
extern void x11FDHandler();
extern bool x11FDPending();
#endif

void EventLoopFileSource::init(int fd, PollEventDelegate callback, uint events)
{
	logMsg("adding fd %d to glib context", fd);
	static GSourceFuncs fdSourceFuncs
	{
		nullptr,
		nullptr,
		[](GSource *source, GSourceFunc, gpointer user_data)
		{
			auto &e = *((EventLoopFileSource*)user_data);
			//logMsg("events for fd: %d", e.fd());
			return (gboolean)e.callback(e.fd_, g_source_query_unix_fd(source, e.tag));
		},
		nullptr
	};
	fd_ = fd;
	var_selfs(callback);
	source = g_source_new(&fdSourceFuncs, sizeof(GSource));
	tag = g_source_add_unix_fd(source, fd, (GIOCondition)events);
	g_source_set_callback(source, nullptr, this, nullptr);
	g_source_attach(source, nullptr);
	g_source_unref(source);
}

#ifdef CONFIG_BASE_X11
void EventLoopFileSource::initX(int fd)
{
	logMsg("adding X fd %d to glib context", fd);
	static GSourceFuncs fdSourceFuncs
	{
		[](GSource *, gint *timeout)
		{
			*timeout = -1;
			return (gboolean)x11FDPending();
		},
		[](GSource *)
		{
			return (gboolean)x11FDPending();
		},
		[](GSource *, GSourceFunc, gpointer)
		{
			//logMsg("events for X fd");
			x11FDHandler();
			return (gboolean)TRUE;
		},
		nullptr
	};
	fd_ = fd;
	source = g_source_new(&fdSourceFuncs, sizeof(GSource));
	tag = g_source_add_unix_fd(source, fd, G_IO_IN);
	g_source_attach(source, nullptr);
	g_source_unref(source);
}
#endif

void EventLoopFileSource::setEvents(uint events)
{
	if(!fd_)
	{
		bug_exit("tried to set events on uninitialized source");
	}
	g_source_modify_unix_fd(source, tag, (GIOCondition)events);
}

int EventLoopFileSource::fd() const
{
	return fd_;
}

void EventLoopFileSource::deinit()
{
	logMsg("removing fd %d from glib context", fd_);
	if(!fd_)
	{
		bug_exit("tried to destroy uninitialized source");
	}
	g_source_destroy(source);
	fd_ = -1;
}

void initMainEventLoop() {}

void runMainEventLoop()
{
	logMsg("entering glib event loop");
	auto mainLoop = g_main_loop_new(nullptr, false);
	g_main_loop_run(mainLoop);
}

}
