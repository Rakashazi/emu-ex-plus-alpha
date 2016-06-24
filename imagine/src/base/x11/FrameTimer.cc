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
#if !defined CONFIG_MACHINE_PANDORA && defined CONFIG_BASE_X11_EGL
#include <EGL/eglextchromium.h>
#endif
#include "internal.hh"

// for fbdev vsync
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

namespace Base
{

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

class OMLFrameTimer
{
private:
	Base::EventLoopFileSource fdSrc;
	#ifndef CONFIG_BASE_X11_EGL
	PFNGLXGETSYNCVALUESOMLPROC getSyncValues = nullptr;
	using syncval_t = int64_t;
	#else
	PFNEGLGETSYNCVALUESCHROMIUMPROC getSyncValues = nullptr;
	using syncval_t = EGLuint64CHROMIUM;
	#endif
	int fd = -1;
	bool requested = false;

public:
	constexpr OMLFrameTimer() {}
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
static FBDevFrameTimer frameTimer;
#elif defined CONFIG_BASE_X11_EGL
static OMLFrameTimer frameTimer;
#else
static SGIFrameTimer frameTimer;
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
	frameTimer.scheduleVSync();
}

void frameTimerCancel()
{
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
			GLContext::swapPresentedBuffers(mainWindow());
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
			{
				Base::GLContextAttributes glAttr;
				glAttr.setMajorVersion(3);
				glAttr.setMinorVersion(3);
				Base::GLBufferConfigAttributes glBuffAttr;
				glBuffAttr.setPixelFormat(Window::defaultPixelFormat());
				bufferConfig = context.makeBufferConfig(glAttr, glBuffAttr);
				context.init(glAttr, bufferConfig);
				if(!context)
				{
					glAttr.setMajorVersion(1);
					glAttr.setMinorVersion(3);
					bufferConfig = context.makeBufferConfig(glAttr, {});
					context.init(glAttr, bufferConfig);
				}
				assert(context);
			}
			Base::Window dummyWindow;
			{
				auto rootWindow = RootWindowOfScreen(mainScreen().xScreen);
				XSetWindowAttributes attr{};
				attr.colormap = XCreateColormap(dpy, rootWindow, bufferConfig.visual, AllocNone);
				dummyWindow.xWin = XCreateWindow(dpy, rootWindow, 0, 0, 1, 1, 0,
					bufferConfig.depth, InputOutput, bufferConfig.visual,
					CWColormap, &attr);
				//logMsg("setting context current");
				GLContext::setCurrent(context, &dummyWindow);
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
			context.deinit();
			GLContext::setCurrent({}, nullptr);
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

#elif !defined CONFIG_MACHINE_PANDORA

bool OMLFrameTimer::init()
{
	if(fd >= 0)
		return true;
	#ifndef CONFIG_BASE_X11_EGL
	getSyncValues = (PFNGLXGETSYNCVALUESOMLPROC)glXGetProcAddress((const GLubyte*)"glXGetSyncValuesOML");
	#else
	GLContext::eglDisplay(); // make sure EGL is initialized
	getSyncValues = (PFNEGLGETSYNCVALUESCHROMIUMPROC)eglGetProcAddress("eglGetSyncValuesCHROMIUM");
	#endif
	if(!getSyncValues)
	{
		#ifndef CONFIG_BASE_X11_EGL
		logErr("error creating frame timer, GLX_OML_sync_control extension is required");
		#else
		logErr("error creating frame timer, EGL_CHROMIUM_sync_control extension is required");
		#endif
		return false;
	}
	fd = timerfd_create(CLOCK_REALTIME, 0);
	assert(fd != -1);
	fdSrc.init(fd,
		[this](int fd, int event)
		{
			{
				uint64_t timesFired;
				int bytes = ::read(fd, &timesFired, 8);
				assert(bytes != -1);
			}
			if(!requested)
				return 1;
			int64_t frameTimeNanos;
			{
				syncval_t ust, msc, sbc;
				#ifndef CONFIG_BASE_X11_EGL
				getSyncValues(dpy, mainWindow().xWin, &ust, &msc, &sbc);
				#else
				getSyncValues(GLContext::eglDisplay(), mainWindow().surface, &ust, &msc, &sbc);
				#endif
				timestamp = ust * 1000;
			}
			requested = false;
			iterateTimes(Screen::screens(), i)
			{
				auto s = Screen::screen(i);
				if(s->isPosted())
				{
					s->frameUpdate(timestamp);
					s->prevFrameTimestamp = timestamp;
				}
			}
			return 1;
		});
	return true;
}

void OMLFrameTimer::deinit()
{
	// TODO
}

void OMLFrameTimer::scheduleVSync()
{
	assert(fd != -1);
	if(requested)
		return;
	requested = true;
	syncval_t nextFrameTimeUSecs;
	{
		syncval_t ust, msc, sbc;
		#ifndef CONFIG_BASE_X11_EGL
		getSyncValues(dpy, mainWindow().xWin, &ust, &msc, &sbc);
		#else
		getSyncValues(GLContext::eglDisplay(), mainWindow().surface, &ust, &msc, &sbc);
		#endif
		// TODO: get real refresh rate, assuming 60Hz
		nextFrameTimeUSecs = ust + (1000000 / (syncval_t)60);
	}
	//logMsg("last frame at %lld, next at %lld, now %lld", (long long)lastFrameTimeUSecs, (long long)nextFrameTimeUSecs, (long long)IG::Time::now().toNs() / 1000);
	int64_t seconds = nextFrameTimeUSecs / 1000000;
	int64_t leftoverUs = nextFrameTimeUSecs % 1000000;
	int64_t leftoverNs = leftoverUs * 1000;
	struct itimerspec vsyncTime{{0, 0}, {seconds, leftoverNs}};
	//logMsg("timerfd @ %lld second(s) and %lld ns", (long long)newTime.it_value.tv_sec, (long long)newTime.it_value.tv_nsec);
	if(timerfd_settime(fd, TIMER_ABSTIME, &vsyncTime, nullptr) != 0)
	{
		logErr("error in timerfd_settime: %s", strerror(errno));
	}
}

void OMLFrameTimer::cancel()
{
	assert(fd != -1);
	if(!requested)
		return;
	requested = false;
	struct itimerspec disable{{0}};
	timerfd_settime(fd, 0, &disable, nullptr);
}

#endif

}
