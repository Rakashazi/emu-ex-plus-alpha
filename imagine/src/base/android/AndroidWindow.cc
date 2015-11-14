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
#include <imagine/base/GLContext.hh>
#include "android.hh"
#include "internal.hh"
#include <imagine/util/fd-utils.h>
#include <android/native_activity.h>
#include <android/native_window_jni.h>
#include <android/looper.h>

namespace Base
{

extern JavaInstMethod<jobject(jobject, jlong)> jPresentation;
static JavaInstMethod<void()> jPresentationShow{};
static JavaInstMethod<void()> jPresentationDeinit{};

Window *deviceWindow()
{
	return Window::window(0);
}

static int winFormatFromEGLConfig(EGLDisplay display, EGLConfig config)
{
	EGLint nId;
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &nId);
	if(!nId)
	{
		nId = WINDOW_FORMAT_RGBA_8888;
		EGLint alphaSize;
		eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &alphaSize);
		if(!alphaSize)
			nId = WINDOW_FORMAT_RGBX_8888;
		EGLint redSize;
		eglGetConfigAttrib(display, config, EGL_RED_SIZE, &redSize);
		if(redSize < 8)
			nId = WINDOW_FORMAT_RGB_565;
		//logWarn("config didn't provide a native format id, guessing %d", nId);
	}
	return nId;
}

static void initPresentationJNI(JNIEnv* env, jobject presentation)
{
	if(jPresentationDeinit)
		return; // already init
	logMsg("Setting up Presentation JNI functions");
	auto cls = env->GetObjectClass(presentation);
	jPresentationShow.setup(env, cls, "show", "()V");
	jPresentationDeinit.setup(env, cls, "deinit", "()V");
	JNINativeMethod method[] =
	{
		{
			"onSurfaceCreated", "(JLandroid/view/Surface;)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong, jobject))
			([](JNIEnv* env, jobject thiz, jlong windowAddr, jobject surface)
			{
				auto nWin = ANativeWindow_fromSurface(env, surface);
				auto &win = *((Window*)windowAddr);
				win.setNativeWindow(nWin);
			})
		},
		{
			"onSurfaceRedrawNeeded", "(J)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong))
			([](JNIEnv* env, jobject thiz, jlong windowAddr)
			{
				auto &win = *((Window*)windowAddr);
				androidWindowNeedsRedraw(win);
			})
		},
		{
			"onSurfaceDestroyed", "(J)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong))
			([](JNIEnv* env, jobject thiz, jlong windowAddr)
			{
				auto &win = *((Window*)windowAddr);
				ANativeWindow_release(win.nativeWindow());
				win.setNativeWindow(nullptr);
			})
		},
	};
	env->RegisterNatives(cls, method, IG::size(method));
}

IG::Point2D<float> Window::pixelSizeAsMM(IG::Point2D<int> size)
{
	auto &s = *screen();
	assert(s.xDPI && s.yDPI);
	float xdpi = surfaceRotationIsStraight(osRotation) ? s.xDPI : s.yDPI;
	float ydpi = surfaceRotationIsStraight(osRotation) ? s.yDPI : s.xDPI;
	return {((float)size.x / xdpi) * 25.4f, ((float)size.y / ydpi) * 25.4f};
}

IG::Point2D<float> Window::pixelSizeAsSMM(IG::Point2D<int> size)
{
	auto &s = *screen();
	assert(s.densityDPI);
	return {((float)size.x / s.densityDPI) * 25.4f, ((float)size.y / s.densityDPI) * 25.4f};
}

bool Window::setValidOrientations(uint oMask)
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
		bcase VIEW_ROTATE_ALL: toSet = 10; // SCREEN_ORIENTATION_FULL_SENSOR
	}
	jSetRequestedOrientation(jEnv(), jBaseActivity, toSet);
	return true;
}

bool Window::requestOrientationChange(uint o)
{
	// no-op, OS manages orientation changes
	return false;
}

PixelFormat Window::defaultPixelFormat()
{
	return ((Config::ARM_ARCH && Config::ARM_ARCH < 7) || androidSDK() < 11) ? PIXEL_FMT_RGB565 : PIXEL_FMT_RGB888;
}

CallResult Window::init(const WindowConfig &config)
{
	if(initialInit)
		return OK;
	BaseWindow::init(config);
	if(!Config::BASE_MULTI_WINDOW && windows())
	{
		bug_exit("no multi-window support");
	}
	#ifdef CONFIG_BASE_MULTI_SCREEN
	screen_ = &config.screen();
	#endif

	initialInit = true;
	#ifdef CONFIG_BASE_MULTI_WINDOW
	window_.push_back(this);
	if(window_.size() > 1)
	{
		logMsg("making presentation window");
		assert(screen() != Screen::screen(0));
		auto env = jEnv();
		jDialog = env->NewGlobalRef(jPresentation(env, Base::jBaseActivity, screen()->aDisplay, (jlong)this));
		initPresentationJNI(env, jDialog);
		jPresentationShow(env, jDialog);
	}
	else
	{
		logMsg("making device window");
	}
	#else
	mainWin = this;
	#endif
	eglConfig = config.glConfig().glConfig;
	pixelFormat = winFormatFromEGLConfig(GLContext::eglDisplay(), eglConfig);
	if(Base::androidSDK() < 11 && this == deviceWindow())
	{
		// In testing with CM7 on a Droid, not setting window format to match
		// what's used in ANativeWindow_setBuffersGeometry() may cause performance issues
		auto env = jEnv();
		if(Config::DEBUG_BUILD)
			logMsg("setting window format to %d (current %d)", pixelFormat, jWinFormat(env, jBaseActivity));
		jSetWinFormat(env, jBaseActivity, pixelFormat);
	}
	// default to screen's size
	updateSize({screen()->width(), screen()->height()});
	contentRect.x2 = width();
	contentRect.y2 = height();
	return OK;
}

void Window::deinit()
{
	assert(this != deviceWindow());
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(jDialog)
	{
		logMsg("deinit presentation");
		if(nWin)
		{
			ANativeWindow_release(nWin);
		}
		setNativeWindow(nullptr);
		auto env = jEnv();
		jPresentationDeinit(env, jDialog);
		env->DeleteGlobalRef(jDialog);
	}
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

bool Window::hasSurface()
{
	return nWin;
}

EGLSurface AndroidWindow::eglSurface()
{
	if(unlikely(surface == EGL_NO_SURFACE && nWin))
	{
		initEGLSurface(GLContext::eglDisplay());
	}
	return surface;
}

bool AndroidWindow::presented()
{
	return presented_;
}

void AndroidWindow::setPresented(bool presented)
{
	presented_ = presented;
}

void AndroidWindow::initEGLSurface(EGLDisplay display)
{
	assert(display != EGL_NO_DISPLAY);
	assert(nWin);
	if(surface != EGL_NO_SURFACE)
	{
		return;
	}
	logMsg("creating EGL surface for native window %p", nWin);
	surface = eglCreateWindowSurface(display, eglConfig, nWin, nullptr);
	assert(surface != EGL_NO_SURFACE);
}

void AndroidWindow::destroyEGLSurface(EGLDisplay display)
{
	if(surface == EGL_NO_SURFACE)
	{
		return;
	}
	if(eglGetCurrentSurface(EGL_DRAW) == surface)
	{
		GLContext::setDrawable(nullptr);
		onGLDrawableChanged.callCopySafe(nullptr);
	}
	logMsg("destroying EGL surface for native window %p", nWin);
	if(eglDestroySurface(display, surface) == EGL_FALSE)
	{
		bug_exit("couldn't destroy surface");
	}
	surface = EGL_NO_SURFACE;
}

void AndroidWindow::updateContentRect(const IG::WindowRect &rect)
{
	contentRect = rect;
	surfaceChange.addContentRectResized();
}

void AndroidWindow::setNativeWindow(ANativeWindow *nWindow)
{
	if(nWindow)
	{
		assert(!nWin);
		nWin = nWindow;
		if(Config::DEBUG_BUILD)
			logMsg("creating window with native visual ID: %d with format: %d", pixelFormat, ANativeWindow_getFormat(nWindow));
		ANativeWindow_setBuffersGeometry(nWindow, 0, 0, pixelFormat);
	}
	else
	{
		((Window*)this)->unpostDraw();
		destroyEGLSurface(GLContext::eglDisplay());
		nWin = nullptr;
	}
}

ANativeWindow *AndroidWindow::nativeWindow()
{
	return nWin;
}

int AndroidWindow::nativePixelFormat()
{
	return pixelFormat;
}

void androidWindowNeedsRedraw(Window &win)
{
	logMsg("window surface redraw needed");
	win.setNeedsDraw(true);
	win.dispatchOnDraw();
	if(!AndroidGLContext::swapBuffersIsAsync())
		GLContext::swapPresentedBuffers(win);
	// On some OS versions like CM7 on the HP Touchpad,
	// the very next frame is rendered incorrectly
	// (as if the window still has its previous size).
	// A similar issue occurs on the stock AT&T Xperia Play
	// when sliding the phone open from sleep with the
	// screen previously in portrait.
	// Post another window draw to work-around this
	win.postDraw();
}

void androidWindowContentRectChanged(Window &win, const IG::WindowRect &rect, const IG::Point2D<int> &winSize)
{
	logMsg("content rect changed: %d:%d:%d:%d in %dx%d",
		rect.x, rect.y, rect.x2, rect.y2, winSize.x, winSize.y);
	win.updateContentRect(rect);
	if(win.updateSize(winSize))
	{
		// If the surface changed size, make sure
		// it's set again with eglMakeCurrent to avoid
		// the context possibly rendering to it with
		// the old size, as occurs on Intel HD Graphics
		onGLDrawableChanged.callCopySafe(nullptr);
	}
	win.postDraw();
}

void Window::setTitle(const char *name) {}

void Window::setAcceptDnd(bool on) {}

}
