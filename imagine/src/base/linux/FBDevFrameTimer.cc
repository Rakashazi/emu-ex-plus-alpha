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
#include <imagine/base/Window.hh>
#include <imagine/time/Time.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include "FBDevFrameTimer.hh"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/eventfd.h>

namespace Base
{

bool FBDevFrameTimer::init(EventLoop loop)
{
	if(fd >= 0)
		return true;
	int fbdev = open("/dev/fb0", O_RDONLY);
	if(fbdev == -1)
	{
		logErr("error creating frame timer, fbdev access is required");
		return false;
	}
	fd = eventfd(0, 0);
	if(fd == -1)
	{
		logErr("error creating eventfd");
		return false;
	}
	sem_init(&sem, 0, 0);
	fdSrc = {fd, loop,
		[this](int fd, int event)
		{
			uint64_t timestamp;
			auto ret = read(fd, &timestamp, sizeof(timestamp));
			assert(ret == sizeof(timestamp));
			//logDMsg("read frame timestamp:%lu", (long unsigned int)timestamp);
			requested = false;
			if(cancelled)
			{
				cancelled = false;
				return true; // frame request was cancelled
			}
			auto &screen = mainScreen();
			assert(screen.isPosted());
			assumeExpr(timestamp >= screen.prevFrameTimestamp);
			if(screen.prevFrameTimestamp)
			{
				/*logDMsg("%lunsecs since last frame (%lu - %lu)",
					(long unsigned int)(timestamp - screen.prevFrameTimestamp),
					(long unsigned int)(timestamp),
					(long unsigned int)(screen.prevFrameTimestamp));*/
			}
			screen.frameUpdate(timestamp);
			screen.prevFrameTimestamp = timestamp;
			return true;
		}};
	IG::makeDetachedThread(
		[this, fbdev]()
		{
			//logMsg("ready to wait for vsync");
			for(;;)
			{
				sem_wait(&sem);
				//logMsg("waiting for vsync");
				int arg = 0;
				ioctl(fbdev, FBIO_WAITFORVSYNC, &arg);
				uint64_t timestamp = IG::Time::now().nSecs();
				//logMsg("got vsync at time %lu", (long unsigned int)timestamp);
				auto ret = write(fd, &timestamp, sizeof(timestamp));
				assert(ret == sizeof(timestamp));
			}
		}
	);
	return true;
}

void FBDevFrameTimer::deinit()
{
	// TODO
}

void FBDevFrameTimer::scheduleVSync()
{
	assert(fd != -1);
	cancelled = false;
	if(requested)
	{
		return;
	}
	requested = true;
	sem_post(&sem);
}

void FBDevFrameTimer::cancel()
{
	cancelled = true;
}

}
