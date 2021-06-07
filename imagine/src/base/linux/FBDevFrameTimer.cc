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

#define LOGTAG "FBDevFrameTimer"
#include <imagine/base/Screen.hh>
#include <imagine/time/Time.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/linux/FBDevFrameTimer.hh>
#include <imagine/util/UniqueFileDescriptor.hh>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/eventfd.h>

namespace Base
{

static UniqueFileDescriptor openDevice()
{
	const char *fbDevPath = "/dev/fb0";
	logMsg("opening device path:%s", fbDevPath);
	return open(fbDevPath, O_RDWR | O_CLOEXEC);
}

FBDevFrameTimer::FBDevFrameTimer(Screen &screen, EventLoop loop)
{
	auto fbdev = openDevice();
	if(fbdev == -1)
	{
		logErr("error opening device:%s", std::system_category().message(errno).c_str());
		return;
	}
	int fd = eventfd(0, 0);
	if(fd == -1)
	{
		logErr("error creating eventfd");
		return;
	}
	sem_init(&sem, 0, 0);
	fdSrc = {"FBDevFrameTimer", fd, loop,
		[this, &screen](int fd, int event)
		{
			eventfd_t timestamp;
			auto ret = read(fd, &timestamp, sizeof(timestamp));
			assert(ret == sizeof(timestamp));
			//logDMsg("read frame timestamp:%lu", (long unsigned int)timestamp);
			requested = false;
			if(cancelled)
			{
				cancelled = false;
				return true; // frame request was cancelled
			}
			assert(screen.isPosted());
			if(screen.frameUpdate(IG::Nanoseconds(timestamp)))
				scheduleVSync();
			return true;
		}};
	IG::makeDetachedThread(
		[this, fd, fbdev = fbdev.release()]()
		{
			//logMsg("ready to wait for vsync");
			for(;;)
			{
				sem_wait(&sem);
				if(quiting)
					break;
				//logMsg("waiting for vsync");
				int arg = 0;
				if(int res = ioctl(fbdev, FBIO_WAITFORVSYNC, &arg);
					res == -1)
				{
					logErr("error in ioctl FBIO_WAITFORVSYNC");
				}
				eventfd_t timestamp = IG::steadyClockTimestamp().count();
				//logMsg("got vsync at time %lu", (long unsigned int)timestamp);
				auto ret = write(fd, &timestamp, sizeof(timestamp));
				assert(ret == sizeof(timestamp));
			}
			close(fbdev);
		}
	);
}

FBDevFrameTimer::~FBDevFrameTimer()
{
	quiting = true;
	sem_post(&sem);
	fdSrc.closeFD();
}

void FBDevFrameTimer::scheduleVSync()
{
	assert(fdSrc.fd() != -1);
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

bool FBDevFrameTimer::testSupport()
{
	auto fbdev = openDevice();
	if(fbdev == -1)
	{
		logErr("error opening device:%s", std::system_category().message(errno).c_str());
		return false;
	}
	// test ioctl FBIO_WAITFORVSYNC
	if(int arg = 0, res = ioctl(fbdev, FBIO_WAITFORVSYNC, &arg);
		res == -1)
	{
		logErr("error in ioctl FBIO_WAITFORVSYNC, cannot use frame timer");
		return false;
	}
	return true;
}

}
