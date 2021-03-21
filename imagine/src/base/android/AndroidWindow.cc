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

#define LOGTAG "Window"
#include "../common/windowPrivate.hh"
#include "android.hh"
#include "internal.hh"
#include <imagine/util/fd-utils.h>
#include <imagine/logger/logger.h>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/sharedLibrary.hh>
#include <android/native_activity.h>
#include <android/native_window_jni.h>
#include <android/looper.h>

namespace Base
{

static JavaInstMethod<jobject(jobject, jlong, jlong)> jPresentation{};
static JavaInstMethod<void()> jPresentationDeinit{};
static JavaInstMethod<void(jint)> jSetWinFormat{};
static int32_t (*ANativeWindow_setFrameRate)(ANativeWindow* window, float frameRate, int8_t compatibility){};

Window *deviceWindow(ApplicationContext app)
{
	return app.window(0);
}

static void initPresentationJNI(JNIEnv* env, jobject presentation)
{
	if(jPresentationDeinit)
		return; // already init
	logMsg("Setting up Presentation JNI functions");
	auto cls = env->GetObjectClass(presentation);
	jPresentationDeinit.setup(env, cls, "deinit", "()V");
	JNINativeMethod method[] =
	{
		{
			"onSurfaceCreated", "(JJLandroid/view/Surface;)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong, jlong, jobject))
			([](JNIEnv* env, jobject thiz, jlong nActivityAddr, jlong windowAddr, jobject surface)
			{
				auto nWin = ANativeWindow_fromSurface(env, surface);
				auto &win = *((Window*)windowAddr);
				win.setNativeWindow((ANativeActivity*)nActivityAddr, nWin);
			})
		},
		{
			"onSurfaceRedrawNeeded", "(JJ)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong, jlong))
			([](JNIEnv* env, jobject thiz, jlong nActivityAddr, jlong windowAddr)
			{
				auto &win = *((Window*)windowAddr);
				androidWindowNeedsRedraw(win, true);
			})
		},
		{
			"onSurfaceDestroyed", "(JJ)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong, jlong))
			([](JNIEnv* env, jobject thiz, jlong nActivityAddr, jlong windowAddr)
			{
				auto &win = *((Window*)windowAddr);
				ANativeWindow_release(win.nativeObject());
				win.setNativeWindow((ANativeActivity*)nActivityAddr, nullptr);
			})
		},
		{
			"onWindowDismiss", "(JJ)V",
			(void*)(void (*)(JNIEnv*, jobject, jlong, jlong))
			([](JNIEnv* env, jobject thiz, jlong nActivityAddr, jlong windowAddr)
			{
				auto &win = *((Window*)windowAddr);
				win.dismiss();
			})
		},
	};
	env->RegisterNatives(cls, method, std::size(method));
}

IG::Point2D<float> Window::pixelSizeAsMM(IG::Point2D<int> size)
{
	auto [xDPI, yDPI] = screen()->dpi();
	assert(xDPI && yDPI);
	float xdpi = surfaceRotationIsStraight(osRotation) ? xDPI : yDPI;
	float ydpi = surfaceRotationIsStraight(osRotation) ? yDPI : xDPI;
	return {((float)size.x / xdpi) * 25.4f, ((float)size.y / ydpi) * 25.4f};
}

IG::Point2D<float> Window::pixelSizeAsSMM(IG::Point2D<int> size)
{
	auto densityDPI = screen()->densityDPI();
	assert(densityDPI);
	return {((float)size.x / densityDPI) * 25.4f, ((float)size.y / densityDPI) * 25.4f};
}

bool Window::setValidOrientations(Orientation oMask)
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
	jSetRequestedOrientation(appContext().mainThreadJniEnv(), appContext().baseActivityObject(), toSet);
	return true;
}

bool Window::requestOrientationChange(Orientation o)
{
	// no-op, OS manages orientation changes
	return false;
}

PixelFormat Window::defaultPixelFormat(ApplicationContext app)
{
	return ((Config::ARM_ARCH && Config::ARM_ARCH < 7) || app.androidSDK() < 11) ? PIXEL_FMT_RGB565 : PIXEL_FMT_RGBA8888;
}

Window::Window(ApplicationContext app, WindowConfig config, InitDelegate onInit_):
	AndroidWindow{app, config}
{
	auto &screen = config.screen(app);
	this->screen_ = &screen;
	auto env = app.mainThreadJniEnv();
	if(app.windows() > 0)
	{
		assert(&screen != app.screen(0));
		if(!jPresentation)
			jPresentation = {env, Base::jBaseActivityCls, "presentation", "(Landroid/view/Display;JJ)Lcom/imagine/PresentationHelper;"};
		jWin = env->NewGlobalRef(jPresentation(env, app.baseActivityObject(), screen.displayObject(),
			(jlong)app.aNativeActivityPtr(), (jlong)this));
		initPresentationJNI(env, jWin);
		type = Type::PRESENTATION;
		logMsg("made presentation window:%p", jWin);
	}
	else
	{
		JavaInstMethod<jobject(jlong)> jSetMainContentView(env, Base::jBaseActivityCls, "setMainContentView", "(J)Landroid/view/Window;");
		jWin = env->NewGlobalRef(jSetMainContentView(env, app.baseActivityObject(), (jlong)this));
		type = Type::MAIN;
		logMsg("made device window:%p", jWin);
		jclass jWindowCls = env->GetObjectClass(jWin);
		jSetWinFormat = {env, jWindowCls, "setFormat", "(I)V"};
	}
	if(config.format())
	{
		setFormat(config.format());
	}
	// default to screen's size
	updateSize({screen.width(), screen.height()});
	contentRect.x2 = width();
	contentRect.y2 = height();
	onInit = onInit_;
}

AndroidWindow::~AndroidWindow()
{
	if(jWin)
	{
		auto env = onExit.appContext().mainThreadJniEnv();
		if(type == Type::PRESENTATION)
		{
			logMsg("dismissing presentation window:%p", jWin);
			jPresentationDeinit(env, jWin);
		}
		env->DeleteGlobalRef(jWin);
	}
	if(nWin)
	{
		ANativeWindow_release(nWin);
	}
}

void Window::show()
{
	postDraw();
}

IG::WindowRect Window::contentBounds() const
{
	return contentRect;
}

bool Window::hasSurface() const
{
	return nWin;
}

void AndroidWindow::updateContentRect(const IG::WindowRect &rect)
{
	contentRect = rect;
	surfaceChange.addContentRectResized();
}

bool Window::operator ==(Window const &rhs) const
{
	return jWin == rhs.jWin;
}

AndroidWindow::operator bool() const
{
	return jWin;
}

void AndroidWindow::setNativeWindow(ApplicationContext app, ANativeWindow *nWindow)
{
	if(nWin)
	{
		nWin = nullptr;
		static_cast<Window*>(this)->dispatchSurfaceDestroyed();
	}
	if(!nWindow)
		return;
	nWin = nWindow;
	static_cast<Window*>(this)->surfaceChange.addCreated();
	if(Config::DEBUG_BUILD)
	{
		logMsg("created window with format visual ID:%d (current:%d)", pixelFormat, ANativeWindow_getFormat(nWindow));
	}
	if(pixelFormat)
	{
		if(app.androidSDK() < 11 && this == deviceWindow(app))
		{
			// In testing with CM7 on a Droid, the surface is re-created in RGBA8888 upon
			// resuming the app no matter what format was used in ANativeWindow_setBuffersGeometry().
			// Explicitly setting the format here seems to fix the problem (Android driver bug?).
			// In case of a mismatch, the surface is usually destroyed & re-created by the OS after this callback.
			if(Config::DEBUG_BUILD)
			{
				logMsg("setting window format to %d (current %d) after surface creation",
					nativePixelFormat(), ANativeWindow_getFormat(nWin));
			}
			jSetWinFormat(app.mainThreadJniEnv(), jWin, pixelFormat);
		}
		ANativeWindow_setBuffersGeometry(nWindow, 0, 0, pixelFormat);
	}
	if(onInit)
	{
		onInit(app, *static_cast<Window*>(this));
		onInit = {};
	}
}

NativeWindow Window::nativeObject() const
{
	return nWin;
}

void Window::setIntendedFrameRate(double rate)
{
	if(appContext().androidSDK() < 30)
		return;
	if(unlikely(!nWin))
		return;
	if(unlikely(!ANativeWindow_setFrameRate))
	{
		auto lib = Base::openSharedLibrary("libnativewindow.so");
		Base::loadSymbol(ANativeWindow_setFrameRate, lib, "ANativeWindow_setFrameRate");
	}
	if(ANativeWindow_setFrameRate(nWin, rate, 0))
	{
		logErr("error in ANativeWindow_setFrameRate() with window:%p rate:%.2f", nWin, rate);
	}
}

void Window::setFormat(NativeWindowFormat fmt)
{
	pixelFormat = fmt;
	if(appContext().androidSDK() < 11 && this == deviceWindow(appContext()))
	{
		// In testing with CM7 on a Droid, not setting window format to match
		// what's used in ANativeWindow_setBuffersGeometry() may cause performance issues
		auto env = appContext().mainThreadJniEnv();
		if(Config::DEBUG_BUILD)
		{
			jclass jWindowCls = env->GetObjectClass(jWin);
			JavaInstMethod<jobject()> jWinAttrs{env, jWindowCls, "getAttributes", "()Landroid/view/WindowManager$LayoutParams;"};
			auto attrs = jWinAttrs(env, jWin);
			jclass jLayoutParamsCls = env->GetObjectClass(attrs);
			auto jFormat = env->GetFieldID(jLayoutParamsCls, "format", "I");
			auto currFmt = env->GetIntField(attrs, jFormat);
			logMsg("setting window format:%d (current:%d)", fmt, currFmt);
		}
		jSetWinFormat(env, jWin, fmt);
	}
	if(nWin)
	{
		if(Config::DEBUG_BUILD)
			logMsg("set window format visual ID:%d (current:%d)", fmt, ANativeWindow_getFormat(nWin));
		ANativeWindow_setBuffersGeometry(nWin, 0, 0, fmt);
	}
}

int AndroidWindow::nativePixelFormat()
{
	return pixelFormat;
}

void androidWindowNeedsRedraw(Window &win, bool sync)
{
	win.setNeedsDraw(true);
	if(!win.appContext().isRunning())
	{
		logMsg("deferring window surface redraw until app resumes");
		return;
	}
	logMsg("window surface redraw needed");
	win.dispatchOnDraw(sync);
	if(sync)
	{
		// On some OS versions like CM7 on the HP Touchpad,
		// the very next frame is rendered incorrectly
		// (as if the window still has its previous size).
		// A similar issue occurs on the stock AT&T Xperia Play
		// when sliding the phone open from sleep with the
		// screen previously in portrait.
		// Post another window draw to work-around this
		win.postDraw();
	}
}

void AndroidWindow::setContentRect(const IG::WindowRect &rect, const IG::Point2D<int> &winSize)
{
	logMsg("content rect changed: %d:%d:%d:%d in %dx%d",
		rect.x, rect.y, rect.x2, rect.y2, winSize.x, winSize.y);
	updateContentRect(rect);
	auto &win = *static_cast<Window*>(this);
	if(win.updateSize(winSize))
	{
		// If the surface changed size, make sure
		// it's set again with eglMakeCurrent to avoid
		// the context possibly rendering to it with
		// the old size, as occurs on Intel HD Graphics
		surfaceChange.addReset();
	}
	win.postDraw();
}

void Window::setTitle(const char *name) {}

void Window::setAcceptDnd(bool on) {}

}
