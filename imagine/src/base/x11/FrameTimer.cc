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
#include <imagine/base/GLContext.hh>
#include <imagine/base/EventLoopFileSource.hh>
#include <imagine/time/Time.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <semaphore.h>
#include "internal.hh"
#include "../linux/DRMFrameTimer.hh"

// for fbdev vsync
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

namespace Base
{

// TODO: remove once is DRMFrameTimer is fully tested
class FBDevFrameTimer
{
private:
	Base::EventLoopFileSource fdSrc;
	int fd = -1;
	sem_t sem{};
	bool requested = false;
	bool cancelled = false;

public:
	constexpr FBDevFrameTimer() {}
	bool init();
	void deinit();
	void scheduleVSync();
	void cancel();

	explicit operator bool() const
	{
		return fd >= 0;
	}
};

#if !defined CONFIG_MACHINE_PANDORA
class SGIFrameTimer
{
private:
	Base::EventLoopFileSource fdSrc;
	int fd = -1;
	sem_t sem{};
	sem_t initSem{};
	bool requested = false;
	bool cancelled = false;
	bool inFrameHandlers = false;
	bool finishing = false;

public:
	constexpr SGIFrameTimer() {}
	bool init();
	void deinit();
	void scheduleVSync();
	void cancel();

	explicit operator bool() const
	{
		return fd >= 0;
	}
};
#endif

#if defined CONFIG_MACHINE_PANDORA
static FBDevFrameTimer frameTimer{};
#elif defined CONFIG_BASE_X11_EGL
static DRMFrameTimer frameTimer{};
#else
static SGIFrameTimer frameTimer{};
#endif

void initFrameTimer()
{
	if(frameTimer)
		return;
	if(!frameTimer.init())
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

bool FBDevFrameTimer::init()
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
	assert(fd != -1);
	sem_init(&sem, 0, 0);
	fdSrc.init(fd,
		[this](int fd, int event)
		{
			bug_exit("TODO: Update to new GLDrawable behavior");
			//GLContext::swapPresentedBuffers(mainWindow());
			uint64_t timestamp;
			auto ret = read(fd, &timestamp, sizeof(timestamp));
			assert(ret == sizeof(timestamp));
			requested = false;
			if(cancelled)
			{
				cancelled = false;
				return 1; // frame request was cancelled
			}
			auto &screen = mainScreen();
			assert(screen.isPosted());
			screen.frameUpdate(timestamp);
			screen.prevFrameTimestamp = timestamp;
			if(!requested && mainWindow().presented)
			{
				// if not drawing next frame but the window was presented
				// re-run this handler after next vsync to swapBuffers()
				scheduleVSync();
				cancel();
			}
			return 1;
		});
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
				//logMsg("got vsync at time %lu", (long unsigned int)frameTimeNanos);
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
		return;
	requested = true;
	sem_post(&sem);
}

void FBDevFrameTimer::cancel()
{
	cancelled = true;
}

#if !defined CONFIG_MACHINE_PANDORA && !defined CONFIG_BASE_X11_EGL

bool SGIFrameTimer::init()
{
	if(fd >= 0)
		return true;
	if(!glXGetProcAddress((const GLubyte*)"glXWaitVideoSyncSGI"))
	{
		logErr("error creating frame timer, GLX_SGI_video_sync extension is required");
		return false;
	}
	fd = eventfd(0, 0);
	assert(fd != -1);
	sem_init(&sem, 0, 0);
	sem_init(&initSem, 0, 0);
	fdSrc.init(fd,
		[this](int fd, int event)
		{
			uint64_t timestamp;
			auto ret = read(fd, &timestamp, sizeof(timestamp));
			assert(ret == sizeof(timestamp));
			requested = false;
			if(cancelled)
			{
				cancelled = false;
				logMsg("cancelled");
				return 1; // frame request was cancelled
			}
			inFrameHandlers = true;
			iterateTimes(Screen::screens(), i)
			{
				auto s = Screen::screen(i);
				if(s->isPosted())
				{
					s->frameUpdate(timestamp);
					s->prevFrameTimestamp = timestamp;
					if(requested)
					{
						sem_post(&sem);
					}
				}
			}
			inFrameHandlers = false;
			return 1;
		});
	IG::makeDetachedThread(
		[this]()
		{
			GLContext context;
			GLBufferConfig bufferConfig;
			std::error_code ec;
			auto display = GLDisplay::makeDefault(ec);
			{
				Base::GLContextAttributes glAttr;
				glAttr.setMajorVersion(3);
				glAttr.setMinorVersion(3);
				Base::GLBufferConfigAttributes glBuffAttr;
				glBuffAttr.setPixelFormat(Window::defaultPixelFormat());
				bufferConfig = context.makeBufferConfig(display, glAttr, glBuffAttr);
				context.init(display, glAttr, bufferConfig);
				if(!context)
				{
					glAttr.setMajorVersion(1);
					glAttr.setMinorVersion(3);
					bufferConfig = context.makeBufferConfig(display, glAttr, {});
					context.init(display, glAttr, bufferConfig);
				}
				assert(context);
			}
			Base::Window dummyWindow;
			{
				auto rootWindow = RootWindowOfScreen(mainScreen().xScreen);
				XSetWindowAttributes attr{};
				attr.colormap = XCreateColormap(dpy, rootWindow, bufferConfig.fmt.visual, AllocNone);
				dummyWindow.xWin = XCreateWindow(dpy, rootWindow, 0, 0, 1, 1, 0,
					bufferConfig.fmt.depth, InputOutput, bufferConfig.fmt.visual,
					CWColormap, &attr);
				Base::GLDrawable drawable{};
				drawable = display.makeDrawable(dummyWindow, bufferConfig, ec);
				//logMsg("setting context current");
				GLContext::setCurrent(display, context, drawable);
			}
			//logMsg("getting glXWaitVideoSyncSGI address");
			auto glXWaitVideoSyncSGI = (PFNGLXWAITVIDEOSYNCSGIPROC)glXGetProcAddress((const GLubyte*)"glXWaitVideoSyncSGI");
			//logMsg("ready to wait for vsync");
			sem_post(&initSem);
			for(;;)
			{
				sem_wait(&sem);
				if(unlikely(finishing))
					break;
				//logMsg("waiting for vsync");
				unsigned int retraces = 0;
				if(glXWaitVideoSyncSGI(1, 0, &retraces) != 0)
				{
					bug_exit("error in glXWaitVideoSyncSGI");
				}
				uint64_t timestamp = IG::Time::now().nSecs();
				//logMsg("got vsync at time %lu", (long unsigned int)timestamp);
				auto ret = write(fd, &timestamp, sizeof(timestamp));
				assert(ret == sizeof(timestamp));
			}
			logMsg("cleaning up frame timer thread");
			context.deinit(display);
			GLContext::setCurrent(display, {}, {});
			dummyWindow.deinit();
			sem_post(&initSem);
		}
	);
	sem_wait(&initSem);
	return true;
}

void SGIFrameTimer::deinit()
{
	if(fd == -1)
		return;
	cancel();
	finishing = true;
	sem_post(&sem);
	sem_wait(&initSem);
	sem_destroy(&initSem);
	fdSrc.deinit();
	close(fd);
	fd = -1;
}

void SGIFrameTimer::scheduleVSync()
{
	assert(fd != -1);
	cancelled = false;
	if(requested)
		return;
	requested = true;
	if(!inFrameHandlers)
		sem_post(&sem);
}

void SGIFrameTimer::cancel()
{
	cancelled = true;
}

#endif

}
