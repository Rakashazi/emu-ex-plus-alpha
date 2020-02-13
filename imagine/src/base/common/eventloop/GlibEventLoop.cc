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
#include <imagine/base/Base.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/string.h>
#include <imagine/util/ScopeGuard.hh>
#include <glib-unix.h>
#ifdef CONFIG_BASE_X11
#include "../../x11/x11.hh"
#endif

namespace Base
{

static __thread bool loopRunning;

#ifdef NDEBUG
GlibFDEventSource::GlibFDEventSource(int fd): fd_{fd} {}
#else
GlibFDEventSource::GlibFDEventSource(const char *debugLabel, int fd): fd_{fd}, debugLabel{debugLabel ? debugLabel : "unnamed"} {}
#endif

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
	#ifndef NDEBUG
	debugLabel = o.debugLabel;
	#endif
	return *this;
}

GlibFDEventSource::~GlibFDEventSource()
{
	deinit();
}

void GlibFDEventSource::deinit()
{
	static_cast<FDEventSource*>(this)->removeFromEventLoop();
}

FDEventSource FDEventSource::makeXServerAddedToEventLoop(int fd, EventLoop loop)
{
	FDEventSource src{"XServer", fd};
	src.addXServerToEventLoop(loop);
	return src;
}

bool FDEventSource::addToEventLoop(EventLoop loop, PollEventDelegate callback, uint32_t events)
{
	static GSourceFuncs fdSourceFuncs
	{
		nullptr,
		nullptr,
		[](GSource *source, GSourceFunc, gpointer userData)
		{
			auto s = (GSource2*)source;
			auto pollFD = (GPollFD*)userData;
			//logMsg("events for source:%p in thread 0x%llx", source, (long long)IG::this_thread::get_id());
			return (gboolean)s->callback(pollFD->fd, g_source_query_unix_fd(source, pollFD));
		},
		nullptr
	};
	if(!loop)
		loop = EventLoop::forThread();
	return makeAndAttachSource(&fdSourceFuncs, callback, (GIOCondition)events, loop.nativeObject());
}

void FDEventSource::addXServerToEventLoop(EventLoop loop)
{
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
	if(!loop)
		loop = EventLoop::forThread();
	makeAndAttachSource(&fdSourceFuncs, {}, G_IO_IN, loop.nativeObject());
}

void FDEventSource::modifyEvents(uint32_t events)
{
	assert(source);
	g_source_modify_unix_fd(source, tag, (GIOCondition)events);
}

void FDEventSource::removeFromEventLoop()
{
	if(source)
	{
		g_source_destroy(source);
		g_source_unref(source);
		source = {};
	}
}

void FDEventSource::setCallback(PollEventDelegate callback)
{
	source->callback = callback;
}

bool FDEventSource::hasEventLoop()
{
	return source;
}

int FDEventSource::fd() const
{
	return fd_;
}

void FDEventSource::closeFD()
{
	if(fd_ == -1)
		return;
	removeFromEventLoop();
	close(fd_);
	fd_ = -1;
}

bool GlibFDEventSource::makeAndAttachSource(GSourceFuncs *fdSourceFuncs,
	PollEventDelegate callback_, GIOCondition events, GMainContext *ctx)
{
	assert(!source);
	auto source = (GSource2*)g_source_new(fdSourceFuncs, sizeof(GSource2));
	auto unrefSource = IG::scopeGuard([&](){ g_source_unref(source); });
	source->callback = callback_;
	tag = g_source_add_unix_fd(source, fd_, events);
	g_source_set_callback(source, nullptr, tag, nullptr);
	if(!g_source_attach(source, ctx))
	{
		logErr("error attaching source with fd:%d (%s)", fd_, label());
		return false;
	}
	unrefSource.cancel();
	this->source = source;
	logMsg("added fd:%d to GMainContext:%p (%s)", fd_, ctx, label());
	return true;
}

const char *GlibFDEventSource::label()
{
	#ifdef NDEBUG
	return nullptr;
	#else
	return debugLabel;
	#endif
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
	logMsg("running GMainContext:%p", mainContext);
	loopRunning = true;
	while(loopRunning)
	{
		if(g_main_context_iteration(mainContext, true))
		{
			//logDMsg("handled events for event loop:%p", mainContext);
		}
	}
	logMsg("GMainContext:%p finished", mainContext);
}

void EventLoop::stop()
{
	loopRunning = false;
	g_main_context_wakeup(mainContext);
}

EventLoop::operator bool() const
{
	return mainContext;
}

}
