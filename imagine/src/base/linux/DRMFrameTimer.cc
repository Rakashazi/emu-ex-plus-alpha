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
#include <imagine/logger/logger.h>
#include <imagine/base/linux/DRMFrameTimer.hh>
#include <imagine/util/memory/UniqueFileDescriptor.hh>
#include <xf86drm.h>
#include <unistd.h>
#include <fcntl.h>

namespace IG
{

constexpr SystemLogger log{"DRMFrameTimer"};

static UniqueFileDescriptor openDevice()
{
	const char *drmCardPath = getenv("KMSDEVICE");
	if(!drmCardPath)
		drmCardPath = "/dev/dri/card0";
	log.info("opening device path:{}", drmCardPath);
	return open(drmCardPath, O_RDWR | O_CLOEXEC, 0);
}

DRMFrameTimer::DRMFrameTimer(Screen &screen, EventLoop loop)
{
	auto fd = openDevice();
	if(fd == -1)
	{
		logErr("error opening device:%s", std::system_category().message(errno).c_str());
		return;
	}
	fdSrc = {std::move(fd), {.debugLabel = "DRMFrameTimer", .eventLoop = loop},
		[this, &screen](int fd, int)
		{
			requested = false;
			if(cancelled)
			{
				cancelled = false;
				return true; // frame request was cancelled
			}
			drmEventContext ctx{};
			ctx.version = DRM_EVENT_CONTEXT_VERSION;
			ctx.vblank_handler =
				[]([[maybe_unused]] int fd, [[maybe_unused]] unsigned int frame, unsigned int sec, unsigned int usec, void* data)
				{
					auto &frameTimer = *((DRMFrameTimer*)data);
					constexpr uint64_t USEC_PER_SEC = 1000000;
					auto uSecs = ((uint64_t)sec * USEC_PER_SEC) + (uint64_t)usec;
					frameTimer.timestamp = IG::Microseconds(uSecs);
				};
			auto err = drmHandleEvent(fd, &ctx);
			if(err)
			{
				log.error("error in drmHandleEvent");
			}
			if(screen.isPosted())
			{
				if(screen.frameUpdate(SteadyClockTimePoint{timestamp}))
					scheduleVSync();
			}
			return true;
		}
	};
}

void DRMFrameTimer::scheduleVSync()
{
	assert(fdSrc.fd() != -1);
	cancelled = false;
	if(requested)
		return;
	requested = true;
	drmVBlank vbl{};
	vbl.request.type = (drmVBlankSeqType)(DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT);
	vbl.request.sequence = 1;
	vbl.request.signal = (unsigned long)this;
	if(int err = drmWaitVBlank(fdSrc.fd(), &vbl);
		err)
	{
		log.error("error in drmWaitVBlank");
	}
}

void DRMFrameTimer::cancel()
{
	cancelled = true;
}

void DRMFrameTimer::setEventsOnThisThread(ApplicationContext)
{
	fdSrc.attach(EventLoop::forThread(), {});
}

bool DRMFrameTimer::testSupport()
{
	int fd = openDevice();
	if(fd == -1)
	{
		log.error("error opening device:{}", std::system_category().message(errno));
		return false;
	}
	// test drmWaitVBlank
	{
		drmVBlank vbl{};
		vbl.request.type = (drmVBlankSeqType)(DRM_VBLANK_RELATIVE);
		vbl.request.sequence = 1;
		if(int err = drmWaitVBlank(fd, &vbl);
			err)
		{
			log.error("error in drmWaitVBlank:{}, cannot use frame timer", std::system_category().message(errno));
			return false;
		}
	}
	return true;
}

}
