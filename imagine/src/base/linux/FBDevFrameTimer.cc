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

#include <imagine/base/Screen.hh>
#include <imagine/time/Time.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/linux/FBDevFrameTimer.hh>
#include <imagine/util/memory/UniqueFileDescriptor.hh>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/eventfd.h>

namespace IG
{

constexpr SystemLogger log{"FBDevFrameTimer"};

static UniqueFileDescriptor openDevice()
{
	const char *fbDevPath = "/dev/fb0";
	log.info("opening device path:{}", fbDevPath);
	return open(fbDevPath, O_RDWR | O_CLOEXEC);
}

FBDevFrameTimer::FBDevFrameTimer(Screen &screen, EventLoop loop):
	fdSrc
	{
		eventfd(0, 0), {.debugLabel = "FBDevFrameTimer", .eventLoop = loop},
		[this, &screen](int fd, int)
		{
			eventfd_t timestamp;
			auto ret = read(fd, &timestamp, sizeof(timestamp));
			assert(ret == sizeof(timestamp));
			//log.debug("read frame timestamp:{}", timestamp);
			requested = false;
			if(cancelled)
			{
				cancelled = false;
				return true; // frame request was cancelled
			}
			if(screen.isPosted())
			{
				if(screen.frameUpdate(SteadyClockTimePoint{Nanoseconds{timestamp}}))
					scheduleVSync();
			}
			return true;
		}
	}
{
	auto fbdev = openDevice();
	if(fbdev == -1)
	{
		log.error("error opening device:{}", std::generic_category().message(errno));
		return;
	}
	thread = std::thread(
		[this, fd = fdSrc.fd(), fbdev = fbdev.release()]()
		{
			//log.info("ready to wait for vsync");
			for(;;)
			{
				sem.acquire();
				if(quiting)
					break;
				//log.info("waiting for vsync");
				int arg = 0;
				if(int res = ioctl(fbdev, FBIO_WAITFORVSYNC, &arg);
					res == -1)
				{
					log.error("error in ioctl FBIO_WAITFORVSYNC");
				}
				eventfd_t timestamp = SteadyClock::now().time_since_epoch().count();
				//log.info("got vsync at time:{}", timestamp);
				auto ret = write(fd, &timestamp, sizeof(timestamp));
				assert(ret == sizeof(timestamp));
			}
			close(fbdev);
		});
}

FBDevFrameTimer::~FBDevFrameTimer()
{
	quiting = true;
	sem.release();
	thread.join();
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
	sem.release();
}

void FBDevFrameTimer::cancel()
{
	cancelled = true;
}

void FBDevFrameTimer::setEventsOnThisThread(ApplicationContext)
{
	fdSrc.attach(EventLoop::forThread(), {});
}

bool FBDevFrameTimer::testSupport()
{
	auto fbdev = openDevice();
	if(fbdev == -1)
	{
		log.error("error opening device:{}", std::generic_category().message(errno));
		return false;
	}
	// test ioctl FBIO_WAITFORVSYNC
	if(int arg = 0, res = ioctl(fbdev, FBIO_WAITFORVSYNC, &arg);
		res == -1)
	{
		log.error("error in ioctl FBIO_WAITFORVSYNC, cannot use frame timer");
		return false;
	}
	return true;
}

}
