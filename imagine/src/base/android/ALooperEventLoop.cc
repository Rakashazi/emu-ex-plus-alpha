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

namespace Base
{

static int pollEventCallback(int fd, int events, void *data)
{
	auto &callback = *((PollEventDelegate*)data);
	return callback(fd, events);
}

FDEventSource::FDEventSource(int fd):
	ALooperFDEventSource{fd}
{}

FDEventSource::FDEventSource(int fd, EventLoop loop, PollEventDelegate callback, uint events):
	FDEventSource{fd}
{
	addToEventLoop(loop, callback, events);
}

FDEventSource::FDEventSource(FDEventSource &&o)
{
	swap(*this, o);
}

FDEventSource &FDEventSource::operator=(FDEventSource o)
{
	swap(*this, o);
	return *this;
}

FDEventSource::~FDEventSource()
{
	removeFromEventLoop();
}

void FDEventSource::swap(FDEventSource &a, FDEventSource &b)
{
	std::swap(a.callback_, b.callback_);
	std::swap(a.looper, b.looper);
	std::swap(a.fd_, b.fd_);
}

bool FDEventSource::addToEventLoop(EventLoop loop, PollEventDelegate callback, uint events)
{
	logMsg("adding fd %d to looper", fd_);
	if(!loop)
		loop = EventLoop::forThread();
	callback_ = std::make_unique<PollEventDelegate>(callback);
	auto res = ALooper_addFd(loop.nativeObject(), fd_, ALOOPER_POLL_CALLBACK, events, pollEventCallback, callback_.get());
	if(res != 1)
	{
		callback_ = {};
		return false;
	}
	looper = loop.nativeObject();
	return true;
}

void FDEventSource::modifyEvents(uint events)
{
	assert(looper);
	ALooper_addFd(looper, fd_, ALOOPER_POLL_CALLBACK, events, pollEventCallback, callback_.get());
}

void FDEventSource::removeFromEventLoop()
{
	if(looper)
	{
		logMsg("removing fd %d from looper", fd_);
		ALooper_removeFd(looper, fd_);
		looper = {};
		callback_ = {};
	}
}

void FDEventSource::setCallback(PollEventDelegate callback)
{
	callback_ = std::make_unique<PollEventDelegate>(callback);
}

bool FDEventSource::hasEventLoop()
{
	return looper;
}

int FDEventSource::fd() const
{
	return fd_;
}

EventLoop EventLoop::forThread()
{
	return {ALooper_forThread()};
}

EventLoop EventLoop::makeForThread()
{
	return {ALooper_prepare(0)};
}

void EventLoop::run()
{
	logMsg("running ALooper:%p", looper);
	ALooper_pollAll(-1, nullptr, nullptr, nullptr);
	logMsg("event loop:%p finished", looper);
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
