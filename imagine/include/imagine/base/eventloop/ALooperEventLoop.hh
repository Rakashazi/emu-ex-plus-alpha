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

#include <imagine/base/eventLoopDefs.hh>
#include <imagine/util/typeTraits.hh>
#include <android/looper.h>
#include <memory>

namespace Base
{

static const int POLLEV_IN = ALOOPER_EVENT_INPUT, POLLEV_OUT = ALOOPER_EVENT_OUTPUT,
	POLLEV_ERR = ALOOPER_EVENT_ERROR, POLLEV_HUP = ALOOPER_EVENT_HANGUP;

struct ALooperFDEventSourceInfo
{
	PollEventDelegate callback{};
	ALooper *looper{};
};

class ALooperFDEventSource
{
public:
	constexpr ALooperFDEventSource() {}
	ALooperFDEventSource(int fd) : ALooperFDEventSource{nullptr, fd} {}
	ALooperFDEventSource(const char *debugLabel, int fd);
	ALooperFDEventSource(ALooperFDEventSource &&o);
	ALooperFDEventSource &operator=(ALooperFDEventSource &&o);
	~ALooperFDEventSource();

protected:
	IG_enableMemberIf(Config::DEBUG_BUILD, const char *, debugLabel){};
	std::unique_ptr<ALooperFDEventSourceInfo> info{};
	int fd_ = -1;

	const char *label() const;
	void deinit();
};

using FDEventSourceImpl = ALooperFDEventSource;

class ALooperEventLoop
{
public:
	constexpr ALooperEventLoop() {}
	constexpr ALooperEventLoop(ALooper *looper): looper{looper} {}
	ALooper *nativeObject() const { return looper; }

protected:
	ALooper *looper{};
};

using EventLoopImpl = ALooperEventLoop;

}
