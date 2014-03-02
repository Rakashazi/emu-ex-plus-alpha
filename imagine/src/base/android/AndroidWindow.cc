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

#define LOGTAG "AndroidWindow"
#include <base/common/windowPrivate.hh>
#include <base/Timer.hh>
#include <base/android/sdk.hh>
#include <base/android/private.hh>
#include <base/android/ASurface.hh>
#include <util/fd-utils.h>
#include <gfx/Gfx.hh>
#include <android/native_activity.h>
#include <android/looper.h>
#include "EGLContextHelper.hh"

namespace Base
{

extern float androidXDPI, androidYDPI;
extern float aDensityDPI;
extern JavaInstMethod<void> jSetWinFormat;
extern JavaInstMethod<int> jWinFormat;
extern JavaInstMethod<void> jSetRequestedOrientation;
extern JavaInstMethod<void> jPostFrame, jUnpostFrame;
extern EGLContextHelper eglCtx;
extern EGLDisplay display;
extern int osOrientation;
extern bool resumeAppOnWindowInit;
extern uint onFrameEventIdle;
extern int onFrameEventFd;
extern jobject frameHelper;
extern bool aHasFocus;
static uint windowSizeChecks = 0;
extern uint appState;
extern TimeSys orientationEventTime;

void onResume(ANativeActivity* activity);
void onPause(ANativeActivity* activity);
void windowNeedsRedraw(ANativeActivity* activity, ANativeWindow *nWin);

void setupEGLConfig()
{
	assert(!eglCtx.isInit()); // should only call before initial window is created
	eglCtx.chooseConfig(display);
	#ifndef NDEBUG
	printEGLConf(display, eglCtx.config);
	logMsg("config value: %p", eglCtx.config);
	#endif
	auto env = eEnv();
	if(Base::androidSDK() < 11)
	{
		// In testing with CM7 on a Droid, not setting window format to match
		// what's used in ANativeWindow_setBuffersGeometry() may cause performance issues
		logMsg("setting window format to %d (current %d)", eglCtx.currentWindowFormat(display), jWinFormat(env, jBaseActivity));
		jSetWinFormat(env, jBaseActivity, eglCtx.currentWindowFormat(display));
	}
}

bool Window::shouldAnimateContentBoundsChange() const
{
	return orientationEventTime && (TimeSys::now() - orientationEventTime > TimeSys::makeWithMSecs(500));
}

IG::Point2D<float> Window::pixelSizeAsMM(IG::Point2D<int> size)
{
	assert(xDPI && yDPI);
	float xdpi = ASurface::isStraightOrientation(osOrientation) ? xDPI : yDPI;
	float ydpi = ASurface::isStraightOrientation(osOrientation) ? yDPI : xDPI;
	return {((float)size.x / xdpi) * 25.4f, ((float)size.y / aDensityDPI) * 25.4f};
}

IG::Point2D<float> Window::pixelSizeAsSMM(IG::Point2D<int> size)
{
	assert(aDensityDPI);
	return {((float)size.x / aDensityDPI) * 25.4f, ((float)size.y / aDensityDPI) * 25.4f};
}

void Window::setDPI(float dpi)
{
	if(dpi == 0) // use device reported DPI
	{
		xDPI = androidXDPI;
		yDPI = androidYDPI;
		logMsg("set DPI from OS %f,%f", (double)xDPI, (double)yDPI);
	}
	else
	{
		xDPI = yDPI = dpi;
		logMsg("set DPI override %f", (double)dpi);
	}
	if(updatePhysicalSizeWithCurrentSize())
		postResize();
}

uint Window::setValidOrientations(uint oMask, bool preferAnimated)
{
	using namespace Base;
	logMsg("requested orientation change to %s", Base::orientationToStr(oMask));
	int toSet = -1;
	switch(oMask)
	{
		bdefault: toSet = -1; // SCREEN_ORIENTATION_UNSPECIFIED
		bcase VIEW_ROTATE_0: toSet = 1; // SCREEN_ORIENTATION_PORTRAIT
		bcase VIEW_ROTATE_90: toSet = 0; // SCREEN_ORIENTATION_LANDSCAPE
		bcase VIEW_ROTATE_180: toSet = (androidSDK() > 8) ? 9 : 1; // SCREEN_ORIENTATION_REVERSE_PORTRAIT
		bcase VIEW_ROTATE_270: toSet = (androidSDK() > 8) ? 8 : 0; // SCREEN_ORIENTATION_REVERSE_LANDSCAPE
		bcase VIEW_ROTATE_90 | VIEW_ROTATE_270: toSet = (androidSDK() > 8) ? 6 : 0; // SCREEN_ORIENTATION_SENSOR_LANDSCAPE
		bcase VIEW_ROTATE_0 | VIEW_ROTATE_180: toSet = (androidSDK() > 8) ? 7 : 1; // SCREEN_ORIENTATION_SENSOR_PORTRAIT
	}
	jSetRequestedOrientation(eEnv(), jBaseActivity, toSet);
	return 1;
}

void Window::setPixelBestColorHint(bool best)
{
	assert(!eglCtx.isInit()); // should only call before initial window is created
	eglCtx.useMaxColorBits = best;
}

bool Window::pixelBestColorHintDefault()
{
	return Base::androidSDK() >= 11 /*&& !eglCtx.has32BppColorBugs*/;
}

void Window::swapBuffers()
{
	//auto now = TimeSys::timeNow();
	eglSwapBuffers(display, surface);
	//auto after = TimeSys::timeNow();
	//logMsg("swap time %f", double(after-now));
}

CallResult Window::init(IG::Point2D<int> pos, IG::Point2D<int> size)
{
	if(mainWin)
	{
		bug_exit("created multiple windows");
	}
	assert(androidXDPI);
	xDPI = androidXDPI;
	yDPI = androidYDPI;
	mainWin = this;
	return OK;
}

void Window::show()
{
	postDraw();
}

static void runPendingInit(ANativeActivity* activity, ANativeWindow* nWin)
{
	Window &win = mainWindow();
	if(!win.ranInit)
	{
		win.ranInit = true;
		appState = APP_RUNNING;
		Gfx::init();
		onWindowInit(win);
		// the following handlers should only ever be called after the initial window init
		activity->callbacks->onResume = onResume;
		activity->callbacks->onPause = onPause;
		handleIntent(activity);
		orientationEventTime = TimeSys::now(); // init with the window creation time
	}
	else if(resumeAppOnWindowInit)
	{
		logMsg("running delayed onResume handler");
		doOnResume(activity);
		resumeAppOnWindowInit = false;
	}
}

IG::WindowRect Window::contentBounds() const
{
	return contentRect;
}

void AndroidWindow::initSurface(EGLDisplay display, EGLConfig config, ANativeWindow *win)
{
	assert(display != EGL_NO_DISPLAY);
	assert(eglCtx.context != EGL_NO_CONTEXT);
	assert(surface == EGL_NO_SURFACE);
	logMsg("creating surface with native visual ID: %d for window with format: %d", eglCtx.currentWindowFormat(display), ANativeWindow_getFormat(win));
	ANativeWindow_setBuffersGeometry(win, 0, 0, eglCtx.currentWindowFormat(display));
	surface = eglCreateWindowSurface(display, config, win, 0);

	if(eglMakeCurrent(display, surface, surface, eglCtx.context) == EGL_FALSE)
	{
		logErr("error in eglMakeCurrent");
		abort();
	}
	//logMsg("window size: %d,%d, from EGL: %d,%d", ANativeWindow_getWidth(win), ANativeWindow_getHeight(win), width(display), height(display));
	//logMsg("window size: %d,%d", ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));

	/*if(eglSwapInterval(display, 1) != EGL_TRUE)
	{
		logErr("error in eglSwapInterval");
	}*/
}

void finishWindowInit(ANativeActivity* activity, ANativeWindow *nWin, bool hasFocus)
{
	if(Base::androidSDK() < 11)
	{
		// In testing with CM7 on a Droid, the surface is re-created in RGBA8888 upon
		// resuming the app no matter what format was used in ANativeWindow_setBuffersGeometry().
		// Explicitly setting the format here seems to fix the problem (Android driver bug?).
		// In case of a mismatch, the surface is usually destroyed & re-created by the OS after this callback.
		logMsg("setting window format to %d (current %d) after surface creation", eglCtx.currentWindowFormat(display), ANativeWindow_getFormat(nWin));
		jSetWinFormat(activity->env, activity->clazz, eglCtx.currentWindowFormat(display));
	}
	Window &win = mainWindow();
	win.nWin = nWin;
	win.initSurface(display, eglCtx.config, nWin);
	win.updateSize({ANativeWindow_getWidth(nWin), ANativeWindow_getHeight(nWin)});
	if(!win.contentRect.x2) // check if contentRect hasn't been set yet
	{
		// assume it equals window size
		win.contentRect.x2 = win.width();
		win.contentRect.y2 = win.height();
		logMsg("window created");
	}
	else
	{
		logMsg("window re-created");
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	win.setNeedsDraw(true);
}

static Base::Screen::OnFrameDelegate windowSizeCheck
{
	[](Screen &screen, FrameTimeBase)
	{
		if(mainWindow().updateSize({ANativeWindow_getWidth(mainWindow().nWin), ANativeWindow_getHeight(mainWindow().nWin)}))
			mainWindow().postResize();
		if(windowSizeChecks)
		{
			windowSizeChecks--;
			logMsg("doing window size check & draw next frame (%d left), current size %d,%d", windowSizeChecks, mainWindow().width(), mainWindow().height());
			screen.addOnFrameDelegate(windowSizeCheck);
			mainWindow().postDraw();
		}
		else
		{
			logMsg("done with window size checks, current size %d,%d", mainWindow().width(), mainWindow().height());
		}
	}
};

// Post extra draws and re-check the window size to avoid incorrect display
// if the window size is out of sync with the view (happens mostly on pre-3.0 OS versions).
// For example, on a stock 2.3 Xperia Play,
// putting the device to sleep in portrait, then sliding the gamepad open
// may cause a view bounds change with swapped window width/height.
// Previously this was also used to avoid issues on some devices
// (ex. Archos Gamepad) if the window size changes from
// an OS UI element (navigation or status bar), but shouldn't
// be an issue since the window now uses a full screen layout
void addOnFrameWindowSizeChecks(uint extraChecks)
{
	windowSizeChecks += extraChecks;
	auto &screen = mainScreen();
	if(!screen.containsOnFrameDelegate(windowSizeCheck))
	{
		screen.addOnFrameDelegate(windowSizeCheck);
	}
}

void windowNeedsRedraw(ANativeActivity* activity, ANativeWindow *nWin)
{
	logMsg("window needs redraw");
	assert(mainWindow().isDrawable());
	Window &win = mainWindow();
	win.postDraw();
	addOnFrameWindowSizeChecks(1);
	runPendingInit(activity, nWin);
}

void windowResized(ANativeActivity* activity, ANativeWindow *nWin)
{
	logMsg("window resized, current size %d,%d", ANativeWindow_getWidth(nWin), ANativeWindow_getHeight(nWin));
	assert(mainWindow().isDrawable());
	Window &win = mainWindow();
	win.postDraw();
	addOnFrameWindowSizeChecks(1);
	runPendingInit(activity, nWin);
}

void contentRectChanged(ANativeActivity* activity, const ARect &rect, ANativeWindow *nWin)
{
	Window &win = mainWindow();
	win.contentRect.x = rect.left; win.contentRect.y = rect.top;
	win.contentRect.x2 = rect.right; win.contentRect.y2 = rect.bottom;
	logMsg("content rect changed to %d:%d:%d:%d, last window size %d,%d",
		rect.left, rect.top, rect.right, rect.bottom, win.width(), win.height());
	if(!nWin)
	{
		logMsg("not posting resize due to uninitialized window");
		return;
	}
	win.postResize();
	addOnFrameWindowSizeChecks(1);
}

bool onFrame(int64 frameTimeNanos, bool forceDraw)
{
	frameUpdate(frameTimeNanos, forceDraw);
	return mainScreen().frameIsPosted();
}

void Window::postDraw()
{
	if(unlikely(!nWin))
	{
		logWarn("cannot post redraw to uninitialized window");
		return;
	}
	setNeedsDraw(true);
	mainScreen().postFrame();
}

void Window::unpostDraw()
{
	setNeedsDraw(false);
}

void Screen::postFrame()
{
	if(!appIsRunning())
	{
		//logMsg("can't post frame when app isn't running");
		return;
	}
	else if(!framePosted)
	{
		framePosted = true;
		if(inFrameHandler)
		{
			//logMsg("posting frame while in frame handler");
		}
		else
		{
			//logMsg("posting frame");
			if(jPostFrame.m)
				jPostFrame(eEnv(), frameHelper);
			else
			{
				uint64_t post = 1;
				auto ret = write(onFrameEventFd, &post, sizeof(post));
				assert(ret == sizeof(post));
			}
		}
	}
}

void Screen::unpostFrame()
{
	if(framePosted)
	{
		framePosted = false;
		if(inFrameHandler)
		{
			//logMsg("un-posting frame while in frame handler");
		}
		else
		{
			//logMsg("un-posting frame");
			if(jUnpostFrame.m)
				jUnpostFrame(eEnv(), frameHelper);
			else
			{
				uint64_t post;
				read(onFrameEventFd, &post, sizeof(post));
				onFrameEventIdle = 1; // force handler to idle since it could already be signaled by epoll
			}
		}
	}
}

void restoreOpenGLContext()
{
	eglCtx.restore(display, mainWindow().surface);
}

void windowDestroyed(ANativeActivity* activity, Window &win)
{
	mainScreen().removeOnFrameDelegate(windowSizeCheck);
	win.unpostDraw();
	if(win.isDrawable())
	{
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		logMsg("destroying window surface");
		eglDestroySurface(display, win.surface);
		win.surface = EGL_NO_SURFACE;
	}
	win.nWin = nullptr;
}

}
