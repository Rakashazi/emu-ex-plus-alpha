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
#include <imagine/util/string.h>
#include <imagine/util/ScopeGuard.hh>
#include <glib-unix.h>

namespace Base
{

GlibFDEventSource::GlibFDEventSource(const char *debugLabel, int fd):
	debugLabel{debugLabel ? debugLabel : "unnamed"},
	fd_{fd}
{}

GlibFDEventSource::GlibFDEventSource(GlibFDEventSource &&o)
{
	*this = std::move(o);
}

GlibFDEventSource &GlibFDEventSource::operator=(GlibFDEventSource &&o)
{
	deinit();
	source = std::exchange(o.source, {});
	tag = std::exchange(o.tag, {});
	fd_ = std::exchange(o.fd_, -1);
	debugLabel = o.debugLabel;
	usingGlibSource = o.usingGlibSource;
	return *this;
}

GlibFDEventSource::~GlibFDEventSource()
{
	deinit();
}

bool FDEventSource::attach(EventLoop loop, GSource *source, uint32_t events)
{
	detach();
	if(!loop)
		loop = EventLoop::forThread();
	return attachGSource(source, (GIOCondition)events, loop.nativeObject());
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
				auto s = (GlibSource*)source;
				auto pollFD = (GPollFD*)userData;
				//logMsg("events for source:%p in thread 0x%llx", source, (long long)IG::this_thread::get_id());
				return (gboolean)s->callback(pollFD->fd, g_source_query_unix_fd(source, pollFD));
			}
		},
		.finalize{},
		.closure_callback{},
		.closure_marshal{},
	};
	auto source = (GlibSource*)g_source_new(&fdSourceFuncs, sizeof(GlibSource));
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
	if(!source)
		return;
	g_source_destroy(source);
	g_source_unref(source);
	source = {};
	usingGlibSource = false;
}

void FDEventSource::setEvents(uint32_t events)
{
	if(!hasEventLoop())
	{
		logErr("trying to set events while not attached to event loop");
		return;
	}
	g_source_modify_unix_fd(source, tag, (GIOCondition)events);
}

void FDEventSource::dispatchEvents(uint32_t events)
{
	assert(usingGlibSource);
	static_cast<GlibSource*>(source)->callback(fd(), events);
}

void FDEventSource::setCallback(PollEventDelegate callback)
{
	if(!hasEventLoop())
	{
		logErr("trying to set callback while not attached to event loop");
		return;
	}
	assert(usingGlibSource);
	static_cast<GlibSource*>(source)->callback = callback;
}

bool FDEventSource::hasEventLoop() const
{
	if(source)
	{
		return !g_source_is_destroyed(source);
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

void FDEventSource::closeFD()
{
	if(fd_ == -1)
		return;
	detach();
	close(fd_);
	fd_ = -1;
}

bool GlibFDEventSource::attachGSource(GSource *source, GIOCondition events, GMainContext *ctx)
{
	tag = g_source_add_unix_fd(source, fd_, events);
	g_source_set_callback(source, nullptr, tag, nullptr);
	if(!g_source_attach(source, ctx))
	{
		logErr("error attaching source with fd:%d (%s)", fd_, label());
		return false;
	}
	this->source = source;
	logMsg("added fd:%d to GMainContext:%p (%s)", fd_, ctx, label());
	return true;
}

void GlibFDEventSource::deinit()
{
	static_cast<FDEventSource*>(this)->detach();
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
		logMsg("made GMainContext:%p for thread:0x%lx", defaultCtx, IG::thisThreadID<long>());
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
