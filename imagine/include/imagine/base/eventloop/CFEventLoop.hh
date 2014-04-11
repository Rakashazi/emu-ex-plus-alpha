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

#include <CoreFoundation/CoreFoundation.h>
#include <imagine/base/eventLoopDefs.hh>

namespace Base
{

static constexpr int UNUSED_EVENT = 0;
static constexpr int POLLEV_IN = kCFFileDescriptorReadCallBack, POLLEV_OUT = kCFFileDescriptorWriteCallBack, POLLEV_ERR = UNUSED_EVENT, POLLEV_HUP = UNUSED_EVENT;

struct CFEventLoopFileSource
{
  PollEventDelegate callback;
  CFFileDescriptorRef fdRef = nullptr;
  CFRunLoopSourceRef src = nullptr;
  int fd_ = -1;

	constexpr CFEventLoopFileSource() {}
};

using EventLoopFileSourceImpl = CFEventLoopFileSource;

}
