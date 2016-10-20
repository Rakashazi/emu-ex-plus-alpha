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
#include <imagine/base/Screen.hh>
#include <imagine/logger/logger.h>
#include "DRMFrameTimer.hh"
#include <xf86drm.h>
#include <unistd.h>
#include <fcntl.h>

namespace Base
{

bool DRMFrameTimer::init(EventLoop loop)
{
	if(fd >= 0)
		return true;
	const char *drmCardPath = getenv("KMSDEVICE");
	if(!drmCardPath)
		drmCardPath = "/dev/dri/card0";
	logMsg("opening DRM device path:%s", drmCardPath);
	fd = open(drmCardPath, O_RDWR, 0);
	if(fd == -1)
	{
		logErr("error creating frame timer, DRM/DRI access is required");
		return false;
	}
	fdSrc = {fd, loop,
		[this](int fd, int event)
		{
			requested = false;
			if(cancelled)
			{
				cancelled = false;
				return 1; // frame request was cancelled
			}
			drmEventContext ctx{};
			ctx.version = DRM_EVENT_CONTEXT_VERSION;
			ctx.vblank_handler =
				[](int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data)
				{
					auto &frameTimer = *((DRMFrameTimer*)data);
					constexpr uint64_t USEC_PER_SEC = 1000000;
					auto uSecs = ((uint64_t)sec * USEC_PER_SEC) + (uint64_t)usec;
					frameTimer.timestamp = IG::Time::makeWithUSecs(uSecs);
				};
			auto err = drmHandleEvent(fd, &ctx);
			if(err)
			{
				bug_exit("error in drmHandleEvent");
			}
			iterateTimes(Screen::screens(), i)
			{
				auto s = Screen::screen(i);
				if(s->isPosted())
				{
					s->frameUpdate(timestamp.nSecs());
					s->prevFrameTimestamp = timestamp.nSecs();
				}
			}
			return 1;
		}};
	return true;
}

void DRMFrameTimer::deinit()
{
	if(fd < 0)
		return;
	fdSrc.removeFromEventLoop();
	close(fd);
	fd = -1;
}

void DRMFrameTimer::scheduleVSync()
{
	assert(fd != -1);
	cancelled = false;
	if(requested)
		return;
	requested = true;
	drmVBlank vbl{};
	vbl.request.type = (drmVBlankSeqType)(DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT);
	vbl.request.sequence = 1;
	vbl.request.signal = (unsigned long)this;
	auto err = drmWaitVBlank(fd, &vbl);
	if(err)
	{
		bug_exit("error in drmWaitVBlank");
	}
}

void DRMFrameTimer::cancel()
{
	cancelled = true;
}

}
