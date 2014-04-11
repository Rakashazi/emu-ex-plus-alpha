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

void EventLoopFileSource::init(int fd, PollEventDelegate callback, uint events)
{
	logMsg("adding fd %d to run loop", fd);
	fd_ = fd;
	var_selfs(callback);
	CFFileDescriptorContext ctx{0, this};
	fdRef = CFFileDescriptorCreate(kCFAllocatorDefault, fd, false,
		[](CFFileDescriptorRef fdRef, CFOptionFlags callbackEventTypes, void *info)
		{
			//logMsg("got fd events: 0x%X", (int)callbackEventTypes);
			auto &e = *((EventLoopFileSource*)info);
			e.callback(e.fd_, callbackEventTypes);
			if(e.fdRef) // re-enable callbacks if fd is still open
				CFFileDescriptorEnableCallBacks(fdRef, callbackEventTypes);
		}, &ctx);
	CFFileDescriptorEnableCallBacks(fdRef, events);
	src = CFFileDescriptorCreateRunLoopSource(kCFAllocatorDefault, fdRef, 0);
	CFRunLoopAddSource(CFRunLoopGetMain(), src, kCFRunLoopDefaultMode);
}

void EventLoopFileSource::setEvents(uint events)
{
	assert(fdRef);
	uint disableEvents = ~events & 0x3;
	if(disableEvents)
		CFFileDescriptorDisableCallBacks(fdRef, disableEvents);
	if(events)
		CFFileDescriptorEnableCallBacks(fdRef, events);
}

int EventLoopFileSource::fd() const
{
	return fd_;
}

void EventLoopFileSource::deinit()
{
	if(fdRef)
	{
		logMsg("removing fd %d from run loop", fd_);
		CFFileDescriptorInvalidate(fdRef);
		CFRelease(fdRef);
		fdRef = nullptr;
		CFRunLoopRemoveSource(CFRunLoopGetMain(), src, kCFRunLoopDefaultMode);
		CFRelease(src);
		src = nullptr;
	}
}

}
