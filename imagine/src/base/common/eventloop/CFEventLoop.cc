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

#include <imagine/base/EventLoop.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>

namespace IG
{

constexpr SystemLogger log{"EventLoop"};

static void eventCallback(CFFileDescriptorRef fdRef, CFOptionFlags callbackEventTypes, void *infoPtr)
{
	//log.debug("got fd events: {:X}", callbackEventTypes);
	auto &info = *((CFFDEventSourceInfo*)infoPtr);
	auto fd = CFFileDescriptorGetNativeDescriptor(fdRef);
	if(info.callback(fd, callbackEventTypes))
	{
		if(info.fdRef) // re-enable callbacks if fd is still open
		{
			CFFileDescriptorEnableCallBacks(fdRef, callbackEventTypes);
		}
	}
	else
	{
		info.detachSource();
	}
}

static void releaseCFFileDescriptor(CFFileDescriptorRef fdRef)
{
	CFFileDescriptorInvalidate(fdRef);
	CFRelease(fdRef);
}

void CFFDEventSourceInfo::detachSource()
{
	if(!src)
		return;
	CFRunLoopRemoveSource(loop, src, kCFRunLoopDefaultMode);
	CFRelease(src);
	src = {};
	loop = {};
}

CFFDEventSource::CFFDEventSource(MaybeUniqueFileDescriptor fd, FDEventSourceDesc, PollEventDelegate del):
	info{fd.get() != -1 ? std::make_unique<CFFDEventSourceInfo>(del) : std::unique_ptr<CFFDEventSourceInfo>{}}
{
	if(fd.get() == -1)
		return;
	CFFileDescriptorContext ctx{.version{}, .info = info.get(), .retain{}, .release{}, .copyDescription{}};
	info->fdRef = CFFileDescriptorCreate(kCFAllocatorDefault, fd.release(), fd.ownsFd(),
		eventCallback, &ctx);
}

CFFDEventSource::CFFDEventSource(CFFDEventSource &&o) noexcept
{
	*this = std::move(o);
}

CFFDEventSource &CFFDEventSource::operator=(CFFDEventSource &&o) noexcept
{
	deinit();
	info = std::move(o.info);
	return *this;
}

CFFDEventSource::~CFFDEventSource()
{
	deinit();
}

bool FDEventSource::attach(EventLoop loop, PollEventFlags events)
{
	if(fd() == -1) [[unlikely]]
	{
		log.error("trying to attach without valid fd");
		return false;
	}
	detach();
	if(!loop)
		loop = EventLoop::forThread();
	if(Config::DEBUG_BUILD)
	{
		log.info("adding fd:{} ({}) to run loop:{}", fd(), debugLabel(), (void*)loop.nativeObject());
	}
	assumeExpr(info);
	events &= 0x3;
	CFFileDescriptorEnableCallBacks(info->fdRef, events);
	info->src = CFFileDescriptorCreateRunLoopSource(kCFAllocatorDefault, info->fdRef, 0);
	CFRunLoopAddSource(loop.nativeObject(), info->src, kCFRunLoopDefaultMode);
	info->loop = loop.nativeObject();
	return true;
}

void FDEventSource::detach()
{
	if(!info || !info->src)
		return;
	if(Config::DEBUG_BUILD)
	{
		log.info("removing fd:{} ({}) from run loop:{}", fd(), debugLabel(), (void*)info->loop);
	}
	info->detachSource();
}

void FDEventSource::setEvents(PollEventFlags events)
{
	if(!hasEventLoop())
	{
		log.error("trying to set events while not attached to event loop");
		return;
	}
	assumeExpr(info);
	events &= 0x3;
	auto disableEvents = ~events;
	if(disableEvents)
		CFFileDescriptorDisableCallBacks(info->fdRef, disableEvents);
	if(events)
		CFFileDescriptorEnableCallBacks(info->fdRef, events);
}

void FDEventSource::dispatchEvents(PollEventFlags events)
{
	assumeExpr(info);
	eventCallback(info->fdRef, events & 0x3, info.get());
}

void FDEventSource::setCallback(PollEventDelegate callback)
{
	assumeExpr(info);
	info->callback = callback;
}

bool FDEventSource::hasEventLoop() const
{
	return info && info->loop;
}

int FDEventSource::fd() const
{
	return info ? CFFileDescriptorGetNativeDescriptor(info->fdRef) : -1;
}

void CFFDEventSource::deinit()
{
	if(!info)
		return;
	static_cast<FDEventSource*>(this)->detach();
	if(info->fdRef)
	{
		releaseCFFileDescriptor(info->fdRef);
		info->fdRef = {};
	}
}

EventLoop EventLoop::forThread()
{
	return {CFRunLoopGetCurrent()};
}

EventLoop EventLoop::makeForThread()
{
	return forThread();
}

void EventLoop::run(const bool& condition)
{
	while(condition)
	{
		CFRunLoopRun();
	}
}

void EventLoop::stop()
{
	CFRunLoopStop(loop);
}

EventLoop::operator bool() const
{
	return loop;
}

}
