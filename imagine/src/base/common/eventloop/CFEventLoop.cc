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
#include <imagine/logger/logger.h>

namespace Base
{

#ifdef NDEBUG
CFFDEventSource::CFFDEventSource(int fd)
#else
CFFDEventSource::CFFDEventSource(const char *debugLabel, int fd): debugLabel{debugLabel ? debugLabel : "unnamed"}
#endif
{
	info = std::make_unique<CFFDEventSourceInfo>();
	CFFileDescriptorContext ctx{0, info.get()};
	info->fdRef = CFFileDescriptorCreate(kCFAllocatorDefault, fd, false,
		[](CFFileDescriptorRef fdRef, CFOptionFlags callbackEventTypes, void *info_)
		{
			//logMsg("got fd events: 0x%X", (int)callbackEventTypes);
			auto &info = *((CFFDEventSourceInfo*)info_);
			auto fd = CFFileDescriptorGetNativeDescriptor(fdRef);
			if(info.callback(fd, callbackEventTypes))
			{
				if(info.fdRef) // re-enable callbacks if fd is still open
					CFFileDescriptorEnableCallBacks(fdRef, callbackEventTypes);
			}
		}, &ctx);
}

CFFDEventSource::CFFDEventSource(CFFDEventSource &&o)
{
	*this = std::move(o);
}

CFFDEventSource &CFFDEventSource::operator=(CFFDEventSource &&o)
{
	deinit();
	info = std::exchange(o.info, {});
	src = std::exchange(o.src, {});
	loop = std::exchange(o.loop, {});
	#ifndef NDEBUG
	debugLabel = o.debugLabel;
	#endif
	return *this;
}

static void releaseCFFileDescriptor(CFFileDescriptorRef fdRef)
{
	CFFileDescriptorInvalidate(fdRef);
	CFRelease(fdRef);
}

CFFDEventSource::~CFFDEventSource()
{
	deinit();
}

void CFFDEventSource::deinit()
{
	static_cast<FDEventSource*>(this)->removeFromEventLoop();
	if(info && info->fdRef)
	{
		releaseCFFileDescriptor(info->fdRef);
	}
}

bool FDEventSource::addToEventLoop(EventLoop loop, PollEventDelegate callback, uint32_t events)
{
	assert(info);
	if(Config::DEBUG_BUILD)
	{
		logMsg("adding fd %d to run loop (%s)", fd(), label());
	}
	info->callback = callback;
	CFFileDescriptorEnableCallBacks(info->fdRef, events);
	src = CFFileDescriptorCreateRunLoopSource(kCFAllocatorDefault, info->fdRef, 0);
	if(!loop)
		loop = EventLoop::forThread();
	CFRunLoopAddSource(loop.nativeObject(), src, kCFRunLoopDefaultMode);
	this->loop = loop.nativeObject();
	return true;
}

void FDEventSource::modifyEvents(uint32_t events)
{
	assert(info);
	uint32_t disableEvents = ~events & 0x3;
	if(disableEvents)
		CFFileDescriptorDisableCallBacks(info->fdRef, disableEvents);
	if(events)
		CFFileDescriptorEnableCallBacks(info->fdRef, events);
}

void FDEventSource::removeFromEventLoop()
{
	if(src)
	{
		if(Config::DEBUG_BUILD)
		{
			logMsg("removing fd %d from run loop (%s)", fd(), label());
		}
		CFRunLoopRemoveSource(loop, src, kCFRunLoopDefaultMode);
		CFRelease(src);
		src = {};
		loop = {};
	}
}

void FDEventSource::setCallback(PollEventDelegate callback)
{
	info->callback = callback;
}

bool FDEventSource::hasEventLoop()
{
	return loop;
}

int FDEventSource::fd() const
{
	return info ? CFFileDescriptorGetNativeDescriptor(info->fdRef) : -1;
}

void FDEventSource::closeFD()
{
	int fd_ = fd();
	if(fd_ == -1)
		return;
	removeFromEventLoop();
	close(fd_);
	releaseCFFileDescriptor(info->fdRef);
	info->fdRef = nullptr;
}

const char *CFFDEventSource::label()
{
	#ifdef NDEBUG
	return nullptr;
	#else
	return debugLabel;
	#endif
}

EventLoop EventLoop::forThread()
{
	return {CFRunLoopGetCurrent()};
}

EventLoop EventLoop::makeForThread()
{
	return forThread();
}

void EventLoop::run()
{
	logMsg("running event loop:%p", loop);
	CFRunLoopRun();
	logMsg("event loop:%p finished", loop);
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
