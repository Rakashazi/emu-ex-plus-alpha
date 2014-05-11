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

extern JavaInstMethod<jobject> jPresentation;
static JavaInstMethod<void> jPresentationShow;
static JavaInstMethod<void> jPresentationDeinit;

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
	if(jPresentationDeinit)
		return; // already init
	logMsg("Setting up Presentation JNI functions");
	auto cls = jEnv->GetObjectClass(presentation);
	jPresentationShow.setup(jEnv, cls, "show", "()V");
	jPresentationDeinit.setup(jEnv, cls, "deinit", "()V");
	JNINativeMethod method[] =
	{
		{
			"onSurfaceCreated", "(JLandroid/view/Surface;)V",
			(void*)(void JNICALL(*)(JNIEnv* env, jobject thiz, jlong windowAddr, jobject surface))
			([](JNIEnv* env, jobject thiz, jlong windowAddr, jobject surface)
			{
				auto nWin = ANativeWindow_fromSurface(env, surface);
				auto &win = *((Window*)windowAddr);
				androidWindowInitSurface(win, nWin);
			})
		},
		{
			"onSurfaceRedrawNeeded", "(J)V",
			(void*)(void JNICALL(*)(JNIEnv* env, jobject thiz, jlong windowAddr))
			([](JNIEnv* env, jobject thiz, jlong windowAddr)
			{
				auto &win = *((Window*)windowAddr);
				androidWindowNeedsRedraw(win);
			})
		},
		{
			"onSurfaceDestroyed", "(J)V",
			(void*)(void JNICALL(*)(JNIEnv* env, jobject thiz, jlong windowAddr))
			([](JNIEnv* env, jobject thiz, jlong windowAddr)
			{
				auto &win = *((Window*)windowAddr);
				ANativeWindow_release(win.nWin);
				androidWindowSurfaceDestroyed(win);
			})
		},
	};
	jEnv->RegisterNatives(cls, method, sizeofArray(method));
}

bool Window::shouldAnimateContentBoundsChange() const
{
	return true;
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
	eglSwapBuffers(display, surface);
}

CallResult Window::init(IG::Point2D<int> pos, IG::Point2D<int> size, WindowInitDelegate onInit)
{
	if(initialInit)
		return OK;
	initDelegates();
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
	// default to screen's size
	updateSize({screen().width(), screen().height()});
	contentRect.x2 = width();
	contentRect.y2 = height();
	if(onInit)
		onInit(*this);
	return OK;
}

void Window::deinit()
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(!initialInit || this == deviceWindow())
	{
		return;
	}
	if(jDialog)
	{
		logMsg("deinit presentation");
		if(nWin)
		{
			ANativeWindow_release(nWin);
		}
		androidWindowSurfaceDestroyed(*this);
		auto jEnv = eEnv();
		jPresentationDeinit(jEnv, jDialog);
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
	return nWin;
}

void AndroidWindow::initEGLSurface(EGLDisplay display, EGLConfig config)
{
	assert(display != EGL_NO_DISPLAY);
	assert(eglCtx.context != EGL_NO_CONTEXT);
	assert(nWin);
	if(surface != EGL_NO_SURFACE)
	{
		return;
	}
	logMsg("creating EGL surface for native window %p", nWin);
	surface = eglCreateWindowSurface(display, config, nWin, nullptr);
	assert(surface != EGL_NO_SURFACE);
}

void AndroidWindow::destroyEGLSurface(EGLDisplay display)
{
	if(surface == EGL_NO_SURFACE)
	{
		return;
	}
	if(drawTargetWindow == this)
	{
		eglCtx.makeCurrentSurfaceless(display);
		drawTargetWindow = nullptr;
	}
	logMsg("destroying EGL surface for native window %p", nWin);
	if(eglDestroySurface(display, surface) == EGL_FALSE)
	{
		bug_exit("couldn't destroy surface");
	}
	surface = EGL_NO_SURFACE;
}

void androidWindowInitSurface(Window &win, ANativeWindow *nWin)
{
	win.nWin = nWin;
	#ifndef NDEBUG
	logMsg("creating window with native visual ID: %d with format: %d", eglCtx.currentWindowFormat(display), ANativeWindow_getFormat(nWin));
	#endif
	ANativeWindow_setBuffersGeometry(nWin, 0, 0, eglCtx.currentWindowFormat(display));
	win.initEGLSurface(display, eglCtx.config);
	win.setNeedsDraw(true);
}

void androidWindowNeedsRedraw(Window &win)
{
	logMsg("window needs redraw event");
	win.postDraw();
}

void androidWindowContentRectChanged(Window &win, const IG::WindowRect &rect, const IG::Point2D<int> &winSize)
{
	logMsg("content rect change event: %d:%d:%d:%d in %dx%d",
		rect.x, rect.y, rect.x2, rect.y2, winSize.x, winSize.y);
	win.contentRect = rect;
	win.surfaceChange.addContentRectResized();
	if(win.updateSize(winSize) && androidSDK() < 19)
	{
		// On some OS versions like CM7 on the HP Touchpad,
		// the very next frame is rendered incorrectly
		// (as if the window still has its previous size).
		// Re-create the EGLSurface to make sure EGL sees
		// the new size.
		win.destroyEGLSurface(display);
		win.initEGLSurface(display, eglCtx.config);
	}
	win.postDraw();
}

void androidWindowSurfaceDestroyed(Window &win)
{
	win.unpostDraw();
	win.destroyEGLSurface(display);
	win.nWin = nullptr;
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

}
