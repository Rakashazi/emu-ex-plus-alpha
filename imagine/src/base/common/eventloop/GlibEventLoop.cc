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

#define LOGTAG "EventLoop"
#include <imagine/base/EventLoop.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/ScopeGuard.hh>
#include <glib.h>
#include <glib-unix.h>

namespace IG
{

void destroyGSource(GSource *src)
{
	logMsg("destroying GSource:%p", src);
	g_source_destroy(src);
	g_source_unref(src);
}

GlibFDEventSource::GlibFDEventSource(const char *debugLabel, MaybeUniqueFileDescriptor fd):
	debugLabel{debugLabel ? debugLabel : "unnamed"},
	fd_{std::move(fd)} {}

bool FDEventSource::attach(EventLoop loop, GSource *source, uint32_t events)
{
	usingGlibSource = false;
	if(!loop)
		loop = EventLoop::forThread();
	tag = g_source_add_unix_fd(source, fd_, static_cast<GIOCondition>(events));
	g_source_set_callback(source, nullptr, tag, nullptr);
	if(!g_source_attach(source, loop.nativeObject()))
	{
		logErr("error attaching fd:%d (%s)", (int)fd_, label());
		return false;
	}
	fdSource.reset(source);
	if(tag)
		logMsg("added fd:%d source:%p to GMainContext:%p (%s)", (int)fd_, source, loop.nativeObject(), label());
	return true;
}

bool FDEventSource::attach(EventLoop loop, PollEventDelegate callback_, uint32_t events)
{
	static GSourceFuncs fdSourceFuncs
	{
		.prepare{},
		.check{},
		.dispatch
		{
			[](GSource *source, GSourceFunc, gpointer userData)
			{
				auto s = (PollEventGSource*)source;
				auto pollFD = (GPollFD*)userData;
				//logMsg("events for source:%p in thread 0x%llx", source, (long long)IG::this_thread::get_id());
				return (gboolean)s->callback(pollFD->fd, g_source_query_unix_fd(source, pollFD));
			}
		},
		.finalize{},
		.closure_callback{},
		.closure_marshal{},
	};
	auto source = (PollEventGSource*)g_source_new(&fdSourceFuncs, sizeof(PollEventGSource));
	source->callback = callback_;
	if(!attach(loop, source, events))
	{
		g_source_unref(source);
		return false;
	}
	usingGlibSource = true;
	return true;
}

void FDEventSource::detach()
{
	fdSource = {};
	usingGlibSource = false;
}

void FDEventSource::setEvents(uint32_t events)
{
	if(!hasEventLoop())
	{
		logErr("trying to set events while not attached to event loop");
		return;
	}
	g_source_modify_unix_fd(fdSource.get(), tag, (GIOCondition)events);
}

void FDEventSource::dispatchEvents(uint32_t events)
{
	assert(usingGlibSource);
	static_cast<PollEventGSource*>(fdSource.get())->callback(fd(), events);
}

void FDEventSource::setCallback(PollEventDelegate callback)
{
	if(!hasEventLoop())
	{
		logErr("trying to set callback while not attached to event loop");
		return;
	}
	assert(usingGlibSource);
	static_cast<PollEventGSource*>(fdSource.get())->callback = callback;
}

bool FDEventSource::hasEventLoop() const
{
	if(fdSource)
	{
		return !g_source_is_destroyed(fdSource.get());
	}
	else
	{
		return false;
	}
}

int FDEventSource::fd() const
{
	return fd_;
}

const char *GlibFDEventSource::label() const
{
	return debugLabel;
}

EventLoop EventLoop::forThread()
{
	return {g_main_context_get_thread_default()};
}

EventLoop EventLoop::makeForThread()
{
	auto defaultCtx = g_main_context_get_thread_default();
	if(defaultCtx)
		return defaultCtx;
	defaultCtx = g_main_context_new();
	if(Config::DEBUG_BUILD)
	{
		logMsg("made GMainContext:%p for thread:%d", defaultCtx, IG::thisThreadId());
	}
	g_main_context_push_thread_default(defaultCtx);
	g_main_context_unref(defaultCtx);
	return defaultCtx;
}

void EventLoop::run()
{
	if(g_main_context_iteration(mainContext, true))
	{
		//logDMsg("handled events for event loop:%p", mainContext);
	}
}

void EventLoop::stop()
{
	g_main_context_wakeup(mainContext);
}

EventLoop::operator bool() const
{
	return mainContext;
}

}
