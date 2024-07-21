#pragma once

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

#include <imagine/config/defs.hh>

#if defined __ANDROID__
#include <imagine/base/eventloop/ALooperEventLoop.hh>
#elif defined __linux__
#include <imagine/base/eventloop/GlibEventLoop.hh>
#define CONFIG_BASE_GLIB
#elif defined __APPLE__
#include <imagine/base/eventloop/CFEventLoop.hh>
#endif

#include <utility>

namespace IG
{

class EventLoop : public EventLoopImpl
{
public:
	using EventLoopImpl::EventLoopImpl;

	constexpr EventLoop() = default;
	static EventLoop forThread();
	static EventLoop makeForThread();
	void run(const bool& condition);
	void stop();
	explicit operator bool() const;
};

struct FDEventSourceDesc
{
	const char* debugLabel{};
	std::optional<EventLoop> eventLoop{};
	PollEventFlags events{pollEventInput};
};

class FDEventSource : public FDEventSourceImpl
{
public:
	FDEventSource(MaybeUniqueFileDescriptor fd, FDEventSourceDesc desc, PollEventDelegate del):
		FDEventSourceImpl{std::move(fd), desc, del},
		debugLabel_{desc.debugLabel ? desc.debugLabel : "unnamed"}
	{
		if(desc.eventLoop)
			attach(*desc.eventLoop, desc.events);
	}
	FDEventSource(): FDEventSource{-1, {}, {}} {}
	bool attach(EventLoop loop = {}, PollEventFlags events = pollEventInput);
	void detach();
	void setEvents(PollEventFlags);
	void dispatchEvents(PollEventFlags);
	void setCallback(PollEventDelegate);
	bool hasEventLoop() const;
	int fd() const;
	const char* debugLabel() const { return debugLabel_; }

protected:
	ConditionalMember<Config::DEBUG_BUILD, const char *> debugLabel_{};
};

}
