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

#include <imagine/base/baseDefs.hh>
#include <imagine/util/used.hh>
#include <imagine/util/memory/UniqueFileDescriptor.hh>
#include <CoreFoundation/CoreFoundation.h>
#include <memory>
#include <utility>

namespace IG
{

constexpr int pollEventInput = kCFFileDescriptorReadCallBack, pollEventOutput = kCFFileDescriptorWriteCallBack,
	pollEventError = 0, pollEventHangUp = 0;

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
	CFFDEventSource(MaybeUniqueFileDescriptor, FDEventSourceDesc, PollEventDelegate);
	CFFDEventSource(CFFDEventSource&&) noexcept;
	CFFDEventSource &operator=(CFFDEventSource&&) noexcept;
	~CFFDEventSource();

protected:
	std::unique_ptr<CFFDEventSourceInfo> info;

	void deinit();
};

using FDEventSourceImpl = CFFDEventSource;

class CFEventLoop
{
public:
	constexpr CFEventLoop() = default;
	constexpr CFEventLoop(CFRunLoopRef loop): loop{loop} {}
	CFRunLoopRef nativeObject() { return loop; }

protected:
	CFRunLoopRef loop{};
};

using EventLoopImpl = CFEventLoop;

}
