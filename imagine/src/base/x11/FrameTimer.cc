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
#include "../linux/DRMFrameTimer.hh"
#include "../linux/FBDevFrameTimer.hh"
#include "../common/SimpleFrameTimer.hh"
#include <imagine/base/Application.hh>
#include <imagine/logger/logger.h>
#include <memory>

namespace Base
{

void XApplication::initFrameTimer(EventLoop loop, Screen &screen)
{
	{
		auto timer = std::make_unique<DRMFrameTimer>(loop, screen);
		if(*timer)
		{
			logMsg("using DRM frame timer");
			frameTimer = std::move(timer);
			return;
		}
	}
	{
		auto timer = std::make_unique<FBDevFrameTimer>(loop, screen);
		if(*timer)
		{
			logMsg("using FBDev frame timer");
			frameTimer = std::move(timer);
			return;
		}
	}
	logMsg("using simple frame timer");
	frameTimer = std::make_unique<SimpleFrameTimer>(loop, screen);
	usingSimpleFrameTimer = true;
}

void XApplication::frameTimerScheduleVSync()
{
	assumeExpr(frameTimer);
	frameTimer->scheduleVSync();
}

void XApplication::frameTimerCancel()
{
	assumeExpr(frameTimer);
	frameTimer->cancel();
}

bool XApplication::frameTimeIsSimulated() const
{
	return usingSimpleFrameTimer;
}

}
