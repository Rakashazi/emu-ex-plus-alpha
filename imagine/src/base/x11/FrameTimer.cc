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

#define LOGTAG "FrameTimer"
#include <imagine/logger/logger.h>
#include "internal.hh"
#include "../linux/DRMFrameTimer.hh"
#include "../linux/FBDevFrameTimer.hh"
#include "../common/SimpleFrameTimer.hh"
#include <memory>

namespace Base
{

std::unique_ptr<FrameTimer> frameTimer{};
static bool usingSimpleFrameTimer = false;

void initFrameTimer(EventLoop loop)
{
	{
		auto timer = std::make_unique<DRMFrameTimer>(loop);
		if(*timer)
		{
			logMsg("using DRM frame timer");
			frameTimer = std::move(timer);
			return;
		}
	}
	{
		auto timer = std::make_unique<FBDevFrameTimer>(loop);
		if(*timer)
		{
			logMsg("using FBDev frame timer");
			frameTimer = std::move(timer);
			return;
		}
	}
	logMsg("using simple frame timer");
	frameTimer = std::make_unique<SimpleFrameTimer>(loop);
	usingSimpleFrameTimer = true;
}

void deinitFrameTimer()
{
	frameTimer.reset();
}

void frameTimerScheduleVSync()
{
	assumeExpr(frameTimer);
	frameTimer->scheduleVSync();
}

void frameTimerCancel()
{
	assumeExpr(frameTimer);
	frameTimer->cancel();
}

bool frameTimeIsSimulated()
{
	return usingSimpleFrameTimer;
}

}
