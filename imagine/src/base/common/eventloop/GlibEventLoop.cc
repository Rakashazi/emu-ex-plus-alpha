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
#include <imagine/util/format.hh>
#include <imagine/util/ScopeGuard.hh>
#include <glib.h>
#include <glib-unix.h>

namespace IG
{

constexpr SystemLogger log{"EventLoop"};

void destroyGSource(GSource* src)
{
	log.info("destroying GSource:{}", (void*)src);
	g_source_destroy(src);
	g_source_unref(src);
}

GlibFDEventSource::GlibFDEventSource(MaybeUniqueFileDescriptor fd, FDEventSourceDesc, PollEventDelegate del):
	source{fd.get() != -1 ? makeSource(del) : UniqueGSource{}},
	fd_{std::move(fd)} {}

bool FDEventSource::attach(EventLoop loop, PollEventFlags events)
{
	if(fd_.get() == -1) [[unlikely]]
	{
		log.error("trying to attach without valid fd");
		return false;
	}
	detach();
	if(!loop)
		loop = EventLoop::forThread();
	tag = g_source_add_unix_fd(source.get(), fd_, static_cast<GIOCondition>(events));
	g_source_set_callback(source.get(), nullptr, tag, nullptr);
	if(!g_source_attach(source.get(), loop.nativeObject()))
	{
		log.error("error attaching fd:{} ({})", fd_.get(), debugLabel());
		detach();
		return false;
	}
	log.info("added fd:{} source:{} to GMainContext:{} ({})", fd_.get(), (void*)source.get(), (void*)loop.nativeObject(), debugLabel());
	return true;
}

void FDEventSource::detach()
{
	if(!hasEventLoop())
		return;
	// Re-create the GSource to detach it
	source = makeSource(getDelegate(source.get()));
	tag = {};
}

void FDEventSource::setEvents(PollEventFlags events)
{
	if(!hasEventLoop())
	{
		log.error("trying to set events while not attached to event loop");
		return;
	}
	g_source_modify_unix_fd(source.get(), tag, (GIOCondition)events);
}

void FDEventSource::dispatchEvents(PollEventFlags events)
{
	getDelegate(source.get())(fd(), events);
}

void FDEventSource::setCallback(PollEventDelegate callback)
{
	getDelegate(source.get()) = callback;
}

bool FDEventSource::hasEventLoop() const
{
	return tag;
}

int FDEventSource::fd() const
{
	return fd_;
}

UniqueGSource GlibFDEventSource::makeSource(PollEventDelegate callback)
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
				//log.debug("events for source:{} in thread {}", (void*)source, (long long)IG::this_thread::get_id());
				return (gboolean)s->callback(pollFD->fd, g_source_query_unix_fd(source, pollFD));
			}
		},
		.finalize{},
		.closure_callback{},
		.closure_marshal{},
	};
	auto source = static_cast<PollEventGSource*>(g_source_new(&fdSourceFuncs, sizeof(PollEventGSource)));
	source->callback = callback;
	return UniqueGSource{source};
}

PollEventDelegate& GlibFDEventSource::getDelegate(GSource* src)
{
	return static_cast<PollEventGSource*>(src)->callback;
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
		log.debug("made GMainContext:{} for thread:{}", (void*)defaultCtx, IG::thisThreadId());
	}
	g_main_context_push_thread_default(defaultCtx);
	g_main_context_unref(defaultCtx);
	return defaultCtx;
}

void EventLoop::run(const bool& condition)
{
	while(condition)
	{
		g_main_context_iteration(mainContext, true);
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
