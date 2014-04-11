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

#include <android/looper.h>
#include <imagine/base/eventLoopDefs.hh>

namespace Base
{

static const int POLLEV_IN = ALOOPER_EVENT_INPUT, POLLEV_OUT = ALOOPER_EVENT_OUTPUT,
	POLLEV_ERR = ALOOPER_EVENT_ERROR, POLLEV_HUP = ALOOPER_EVENT_HANGUP;

class ALooperEventLoopFileSource
{
public:
	PollEventDelegate callback;
	int fd_ = -1;

	constexpr ALooperEventLoopFileSource() {}
};
using EventLoopFileSourceImpl = ALooperEventLoopFileSource;

}
