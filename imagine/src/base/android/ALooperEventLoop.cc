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
#include <imagine/logger/logger.h>
#include <unistd.h>

namespace IG
{

static int eventCallback(int fd, int events, void *data)
{
	auto &info = *((ALooperFDEventSourceInfo*)data);
	bool keep = info.callback(fd, events);
	if(!keep)
	{
		info.looper = {};
	}
	return keep;
}

ALooperFDEventSource::ALooperFDEventSource(const char *debugLabel, MaybeUniqueFileDescriptor fd):
	debugLabel{debugLabel ? debugLabel : "unnamed"},
	info{std::make_unique<ALooperFDEventSourceInfo>()},
	fd_{std::move(fd)}
{}

ALooperFDEventSource::ALooperFDEventSource(ALooperFDEventSource &&o) noexcept
{
	*this = std::move(o);
}

ALooperFDEventSource &ALooperFDEventSource::operator=(ALooperFDEventSource &&o) noexcept
{
	deinit();
	info = std::move(o.info);
	fd_ = std::move(o.fd_);
	debugLabel = o.debugLabel;
	return *this;
}

ALooperFDEventSource::~ALooperFDEventSource()
{
	deinit();
}

bool FDEventSource::attach(EventLoop loop, PollEventDelegate callback, uint32_t events)
{
	logMsg("adding fd:%d to looper:%p (%s)", (int)fd_, loop.nativeObject(), label());
	assumeExpr(info);
	detach();
	if(!loop)
		loop = EventLoop::forThread();
	if(auto res = ALooper_addFd(loop.nativeObject(), fd_, ALOOPER_POLL_CALLBACK, events, eventCallback, info.get());
		res != 1)
	{
		return false;
	}
	info->callback = callback;
	info->looper = loop.nativeObject();
	return true;
}

void FDEventSource::detach()
{
	if(!info || !info->looper)
		return;
	logMsg("removing fd %d from looper (%s)", (int)fd_, label());
	ALooper_removeFd(info->looper, fd_);
	info->looper = {};
}

void FDEventSource::setEvents(uint32_t events)
{
	if(!hasEventLoop())
	{
		logErr("trying to set events while not attached to event loop");
		return;
	}
	ALooper_addFd(info->looper, fd_, ALOOPER_POLL_CALLBACK, events, eventCallback, info.get());
}

void FDEventSource::dispatchEvents(uint32_t events)
{
	eventCallback(fd(), events, info.get());
}

void FDEventSource::setCallback(PollEventDelegate callback)
{
	if(!hasEventLoop())
	{
		logErr("trying to set callback while not attached to event loop");
		return;
	}
	info->callback = callback;
}

bool FDEventSource::hasEventLoop() const
{
	assumeExpr(info);
	return info->looper;
}

int FDEventSource::fd() const
{
	return fd_;
}

void ALooperFDEventSource::deinit()
{
	static_cast<FDEventSource*>(this)->detach();
}

const char *ALooperFDEventSource::label() const
{
	return debugLabel;
}

EventLoop EventLoop::forThread()
{
	return {ALooper_forThread()};
}

EventLoop EventLoop::makeForThread()
{
	return {ALooper_prepare(0)};
}

static const char *aLooperPollResultStr(int res)
{
	switch(res)
	{
		case ALOOPER_POLL_CALLBACK: return "Callback";
		case ALOOPER_POLL_ERROR: return "Error";
		case ALOOPER_POLL_TIMEOUT: return "Timeout";
		case ALOOPER_POLL_WAKE: return "Wake";
	}
	return "Unknown";
}

void EventLoop::run()
{
	int res = ALooper_pollAll(-1, nullptr, nullptr, nullptr);
	if(res != ALOOPER_POLL_WAKE)
		logDMsg("ALooper_pollAll returned:%s", aLooperPollResultStr(res));
}

void EventLoop::stop()
{
	ALooper_wake(looper);
}

EventLoop::operator bool() const
{
	return looper;
}

}
