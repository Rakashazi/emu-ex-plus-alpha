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

namespace Base
{

#if defined CONFIG_MACHINE_PANDORA
using SysFrameTimer = FBDevFrameTimer;
#else
using SysFrameTimer = DRMFrameTimer;
#endif

static SysFrameTimer frameTimer{};

void initFrameTimer(EventLoop loop)
{
	if(frameTimer)
		return;
	if(!frameTimer.init(loop))
	{
		exit(1);
	}
}

void deinitFrameTimer()
{
	frameTimer.deinit();
}

void frameTimerScheduleVSync()
{
	if(frameTimer)
		frameTimer.scheduleVSync();
}

void frameTimerCancel()
{
	if(frameTimer)
		frameTimer.cancel();
}

}
