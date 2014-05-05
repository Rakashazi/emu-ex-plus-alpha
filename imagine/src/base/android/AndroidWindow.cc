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
#include "../common/windowPrivate.hh"
#include "../common/screenPrivate.hh"
#include <imagine/base/Timer.hh>
#include <imagine/base/android/sdk.hh>
#include "private.hh"
#include "ASurface.hh"
#include <imagine/util/fd-utils.h>
#include <imagine/gfx/Gfx.hh>
#include <android/native_activity.h>
#include <android/native_window_jni.h>
#include <android/looper.h>
#include "androidBase.hh"

namespace Base
{

static uint windowSizeChecks = 0;

void onResume(ANativeActivity* activity);
void onPause(ANativeActivity* activity);
void windowNeedsRedraw(Window &win, ANativeActivity* activity);
void finishWindowInit(Window &win, ANativeActivity* activity, ANativeWindow *nWin, bool hasFocus);
void windowResized(Window &win, ANativeActivity* activity);
void windowDestroyed(Window &win, ANativeActivity* activity);
extern JavaInstMethod<jobject> jPresentation;
static JavaInstMethod<void> jPresentationShow;
static JavaInstMethod<void> jPresentationDismiss;

Window *deviceWindow()
{
	return Window::window(0);
}

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

static void initPresentationJNI(JNIEnv* jEnv, jobject presentation)
{
	if(jPresentationDismiss)
		return; // already init
	logMsg("Setting up Presentation JNI functions");
	auto cls = jEnv->GetObjectClass(presentation);
	jPresentationShow.setup(jEnv, cls, "show", "()V");
	jPresentationDismiss.setup(jEnv, cls, "dismiss", "()V");
	JNINativeMethod method[] =
	{
		{
			"onSurfaceCreated", "(JLandroid/view/Surface;)V",
			(void*)(void JNICALL(*)(JNIEnv* env, jobject thiz, jlong windowAddr, jobject surface))
			([](JNIEnv* env, jobject thiz, jlong windowAddr, jobject surface)
			{
				auto nWin = ANativeWindow_fromSurface(env, surface);
				auto &win = *((Window*)windowAddr);
				finishWindowInit(win, baseActivity, nWin, false);
			})
		},
		{
			"onSurfaceChanged", "(J)V",
			(void*)(void JNICALL(*)(JNIEnv* env, jobject thiz, jlong windowAddr))
			([](JNIEnv* env, jobject thiz, jlong windowAddr)
			{
				auto &win = *((Window*)windowAddr);
				windowResized(win, baseActivity);
			})
		},
		{
			"onSurfaceRedrawNeeded", "(J)V",
			(void*)(void JNICALL(*)(JNIEnv* env, jobject thiz, jlong windowAddr))
			([](JNIEnv* env, jobject thiz, jlong windowAddr)
			{
				auto &win = *((Window*)windowAddr);
				windowNeedsRedraw(win, baseActivity);
			})
		},
		{
			"onSurfaceDestroyed", "(J)V",
			(void*)(void JNICALL(*)(JNIEnv* env, jobject thiz, jlong windowAddr))
			([](JNIEnv* env, jobject thiz, jlong windowAddr)
			{
				auto &win = *((Window*)windowAddr);
				auto nWin = win.nWin;
				windowDestroyed(win, baseActivity);
				if(nWin)
				{
					ANativeWindow_release(nWin);
				}
			})
		},
	};
	jEnv->RegisterNatives(cls, method, sizeofArray(method));
}

bool Window::shouldAnimateContentBoundsChange() const
{
	return orientationEventTime && (TimeSys::now() - orientationEventTime > TimeSys::makeWithMSecs(500));
}

IG::Point2D<float> Window::pixelSizeAsMM(IG::Point2D<int> size)
{
	assert(screen().xDPI && screen().yDPI);
	float xdpi = ASurface::isStraightOrientation(osOrientation) ? screen().xDPI : screen().yDPI;
	float ydpi = ASurface::isStraightOrientation(osOrientation) ? screen().yDPI : screen().xDPI;
	return {((float)size.x / xdpi) * 25.4f, ((float)size.y / ydpi) * 25.4f};
}

IG::Point2D<float> Window::pixelSizeAsSMM(IG::Point2D<int> size)
{
	assert(screen().densityDPI);
	return {((float)size.x / screen().densityDPI) * 25.4f, ((float)size.y / screen().densityDPI) * 25.4f};
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
		bcase VIEW_ROTATE_180: toSet = 9; // SCREEN_ORIENTATION_REVERSE_PORTRAIT
		bcase VIEW_ROTATE_270: toSet = 8; // SCREEN_ORIENTATION_REVERSE_LANDSCAPE
		bcase VIEW_ROTATE_90 | VIEW_ROTATE_270: toSet = 6; // SCREEN_ORIENTATION_SENSOR_LANDSCAPE
		bcase VIEW_ROTATE_0 | VIEW_ROTATE_180: toSet = 7; // SCREEN_ORIENTATION_SENSOR_PORTRAIT
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

void Window::setPresentInterval(int interval)
{
	// TODO
}

void Window::swapBuffers()
{
	#ifndef NDEBUG
	assert(eglCtx.verify());
	#endif
	//auto now = TimeSys::timeNow();
	eglSwapBuffers(display, surface);
	//auto after = TimeSys::timeNow();
	//logMsg("swap time %f", double(after-now));
}

CallResult Window::init(IG::Point2D<int> pos, IG::Point2D<int> size)
{
	if(initialInit)
		return OK;
	if(!Config::BASE_MULTI_WINDOW && windows())
	{
		bug_exit("no multi-window support");
	}
	#ifdef CONFIG_BASE_MULTI_SCREEN
	screen_ = &mainScreen();
	#endif

	initialInit = true;
	#ifdef CONFIG_BASE_MULTI_WINDOW
	window_.push_back(this);
	if(window_.size() > 1 && Screen::screens() > 1)
	{
		logMsg("making window on external screen");
		auto jEnv = eEnv();
		screen_ = Screen::screen(1);
		jDialog = jEnv->NewGlobalRef(jPresentation(jEnv, Base::jBaseActivity, Screen::screen(1)->aDisplay, this));
		initPresentationJNI(jEnv, jDialog);
		jPresentationShow(jEnv, jDialog);
	}
	#else
	mainWin = this;
	#endif
	return OK;
}


void Window::deinit()
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(!initialInit || this == deviceWindow())
	{
		return;
	}
	auto nWin_ = nWin;
	windowDestroyed(*this, baseActivity);
	if(nWin_)
	{
		ANativeWindow_release(nWin_);
	}
	if(jDialog)
	{
		auto jEnv = eEnv();
		jPresentationDismiss(jEnv, jDialog);
		jEnv->DeleteGlobalRef(jDialog);
	}
	window_.remove(this);
	*this = {};
	#endif
}

void Window::show()
{
	postDraw();
}

static void runPendingInit(Window &win, ANativeActivity* activity)
{
	if(!win.ranInit)
	{
		win.ranInit = true;
		appState = APP_RUNNING;
		onWindowInit(win);
		// the following handlers should only ever be called after the initial window init
		activity->callbacks->onResume = onResume;
		activity->callbacks->onPause = onPause;
		handleIntent(activity);
		orientationEventTime = TimeSys::now(); // init with the window creation time
	}
}

IG::WindowRect Window::contentBounds() const
{
	return contentRect;
}

void Window::setSurfaceCurrent()
{
	assert(eglCtx.context != EGL_NO_CONTEXT);
	assert(surface != EGL_NO_SURFACE);
	//logMsg("setting window 0x%p surface current", nWin);
	if(eglMakeCurrent(display, surface, surface, eglCtx.context) == EGL_FALSE)
	{
		bug_exit("unable to set window 0x%p surface current", nWin);
	}
}

bool Window::hasSurface()
{
	return isDrawable();
}

void AndroidWindow::initSurface(EGLDisplay display, EGLConfig config, ANativeWindow *win)
{
	assert(display != EGL_NO_DISPLAY);
	assert(eglCtx.context != EGL_NO_CONTEXT);
	assert(surface == EGL_NO_SURFACE);
	logMsg("creating surface with native visual ID: %d for window with format: %d", eglCtx.currentWindowFormat(display), ANativeWindow_getFormat(win));
	ANativeWindow_setBuffersGeometry(win, 0, 0, eglCtx.currentWindowFormat(display));
	surface = eglCreateWindowSurface(display, config, win, 0);

	//logMsg("window size: %d,%d, from EGL: %d,%d", ANativeWindow_getWidth(win), ANativeWindow_getHeight(win), width(display), height(display));
	//logMsg("window size: %d,%d", ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));
}

void finishWindowInit(Window &win, ANativeActivity* activity, ANativeWindow *nWin, bool hasFocus)
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
	win.setNeedsDraw(true);
}

static Base::Screen::OnFrameDelegate windowSizeCheck
{
	[](Screen &screen, FrameTimeBase)
	{
		auto &win = *deviceWindow();
		if(win.updateSize({ANativeWindow_getWidth(win.nWin), ANativeWindow_getHeight(win.nWin)}))
			win.postResize();
		if(windowSizeChecks)
		{
			windowSizeChecks--;
			logMsg("doing window size check & draw next frame (%d left), current size %d,%d", windowSizeChecks, win.width(), win.height());
			screen.addOnFrameDelegate(windowSizeCheck);
			win.postDraw();
		}
		else
		{
			logMsg("done with window size checks, current size %d,%d", win.width(), win.height());
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
void addOnFrameWindowSizeChecks(Screen &screen, uint extraChecks)
{
	windowSizeChecks += extraChecks;
	if(!screen.containsOnFrameDelegate(windowSizeCheck))
	{
		screen.addOnFrameDelegate(windowSizeCheck);
	}
}

void windowNeedsRedraw(Window &win, ANativeActivity* activity)
{
	logMsg("window needs redraw");
	assert(win.isDrawable());
	win.postDraw();
	if(&win == deviceWindow())
	{
		addOnFrameWindowSizeChecks(win.screen(), 1);
		runPendingInit(win, activity);
	}
	else
	{
		if(!win.ranInit)
		{
			win.ranInit = true;
			onWindowInit(win);
		}
	}
}

void windowResized(Window &win, ANativeActivity* activity)
{
	logMsg("window resized, current size %d,%d", ANativeWindow_getWidth(win.nWin), ANativeWindow_getHeight(win.nWin));
	assert(win.isDrawable());
	win.postDraw();
	if(&win == deviceWindow())
	{
		addOnFrameWindowSizeChecks(win.screen(), 1);
		runPendingInit(win, activity);
	}
	else
	{
		if(!win.ranInit)
		{
			win.ranInit = true;
			onWindowInit(win);
		}
		win.postResize();
	}
}

void contentRectChanged(ANativeActivity* activity, const ARect &rect, ANativeWindow *nWin)
{
	Window &win = *deviceWindow();
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
	addOnFrameWindowSizeChecks(win.screen(), 1);
}

void Window::postDraw()
{
	if(hasSurface())
	{
		setNeedsDraw(true);
		screen().postFrame();
	}
}

void Window::unpostDraw()
{
	setNeedsDraw(false);
}

void restoreOpenGLContext()
{
	if(!eglCtx.verify())
	{
		logMsg("context not current, setting now");
		eglCtx.makeCurrentSurfaceless(display);
		drawTargetWindow = nullptr;
	}
}

void windowDestroyed(Window &win, ANativeActivity* activity)
{
	win.screen().removeOnFrameDelegate(windowSizeCheck);
	win.unpostDraw();
	if(drawTargetWindow == &win)
	{
		eglCtx.makeCurrentSurfaceless(display);
		drawTargetWindow = nullptr;
	}
	if(win.isDrawable())
	{
		logMsg("destroying window surface");
		eglDestroySurface(display, win.surface);
		win.surface = EGL_NO_SURFACE;
	}
	win.nWin = nullptr;
}

}
