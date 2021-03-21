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
#include <CoreFoundation/CoreFoundation.h>
#include <memory>

namespace Base
{

static constexpr int UNUSED_EVENT = 0;
static constexpr int POLLEV_IN = kCFFileDescriptorReadCallBack, POLLEV_OUT = kCFFileDescriptorWriteCallBack, POLLEV_ERR = UNUSED_EVENT, POLLEV_HUP = UNUSED_EVENT;

struct CFFDEventSourceInfo
{
	PollEventDelegate callback{};
	CFFileDescriptorRef fdRef{};
	CFRunLoopSourceRef src{};
	CFRunLoopRef loop{};

	void detachSource();
};

class CFFDEventSource
{
public:
	constexpr CFFDEventSource() {}
	CFFDEventSource(int fd) : CFFDEventSource{nullptr, fd} {}
	CFFDEventSource(const char *debugLabel, int fd);
	CFFDEventSource(CFFDEventSource &&o);
	CFFDEventSource &operator=(CFFDEventSource &&o);
	~CFFDEventSource();

protected:
	IG_enableMemberIf(Config::DEBUG_BUILD, const char *, debugLabel){};
	std::unique_ptr<CFFDEventSourceInfo> info{};

	const char *label() const;
	void deinit();
};

using FDEventSourceImpl = CFFDEventSource;

class CFEventLoop
{
public:
	constexpr CFEventLoop() {}
	constexpr CFEventLoop(CFRunLoopRef loop): loop{loop} {}
	CFRunLoopRef nativeObject() { return loop; }

protected:
	CFRunLoopRef loop{};
};

using EventLoopImpl = CFEventLoop;

}
