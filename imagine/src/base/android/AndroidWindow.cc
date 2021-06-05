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
#include "android.hh"
#include <imagine/logger/logger.h>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/sharedLibrary.hh>
#include <android/native_activity.h>
#include <android/native_window_jni.h>
#include <android/looper.h>

namespace Base
{

static JNI::InstMethod<jobject(jobject, jlong, jlong)> jPresentation{};
static JNI::InstMethod<void()> jPresentationDeinit{};
static int32_t (*ANativeWindow_setFrameRate)(ANativeWindow* window, float frameRate, int8_t compatibility){};

static void initPresentationJNI(JNIEnv* env, jobject presentation)
{
	if(jPresentationDeinit)
		return; // already init
	logMsg("Setting up Presentation JNI functions");
	auto cls = env->GetObjectClass(presentation);
	jPresentationDeinit = {env, cls, "deinit", "()V"};
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
				win.systemRequestsRedraw(true);
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
	auto osRotation = application().currentRotation();
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
	auto maskToOrientation = [](Orientation oMask)
		{
			switch(oMask)
			{
				default: return -1; // SCREEN_ORIENTATION_UNSPECIFIED
				case VIEW_ROTATE_0: return 1; // SCREEN_ORIENTATION_PORTRAIT
				case VIEW_ROTATE_90: return 0; // SCREEN_ORIENTATION_LANDSCAPE
				case VIEW_ROTATE_180: return 9; // SCREEN_ORIENTATION_REVERSE_PORTRAIT
				case VIEW_ROTATE_270: return 8; // SCREEN_ORIENTATION_REVERSE_LANDSCAPE
				case VIEW_ROTATE_90 | VIEW_ROTATE_270: return 6; // SCREEN_ORIENTATION_SENSOR_LANDSCAPE
				case VIEW_ROTATE_0 | VIEW_ROTATE_180: return 7; // SCREEN_ORIENTATION_SENSOR_PORTRAIT
				case VIEW_ROTATE_ALL: return 10; // SCREEN_ORIENTATION_FULL_SENSOR
			}
		};
	int toSet = maskToOrientation(oMask);
	application().setRequestedOrientation(appContext().mainThreadJniEnv(), appContext().baseActivityObject(), toSet);
	return true;
}

bool Window::requestOrientationChange(Orientation o)
{
	// no-op, OS manages orientation changes
	return false;
}

IG::PixelFormat ApplicationContext::defaultWindowPixelFormat() const
{
	return ((Config::ARM_ARCH && Config::ARM_ARCH < 7) || androidSDK() < 11) ? PIXEL_FMT_RGB565 : PIXEL_FMT_RGBA8888;
}

Window::Window(ApplicationContext ctx, WindowConfig config, InitDelegate onInit_):
	AndroidWindow{ctx, config}
{
	auto &screen = config.screen(ctx);
	this->screen_ = &screen;
	auto env = ctx.mainThreadJniEnv();
	auto baseActivity = ctx.baseActivityObject();
	if(ctx.windows().size())
	{
		assert(screen != ctx.mainScreen());
		if(!jPresentation)
			jPresentation = {env, baseActivity, "presentation", "(Landroid/view/Display;JJ)Lcom/imagine/PresentationHelper;"};
		jWin = {env, jPresentation(env, baseActivity, screen.displayObject(),
			(jlong)ctx.aNativeActivityPtr(), (jlong)this)};
		initPresentationJNI(env, jWin);
		type = Type::PRESENTATION;
		logMsg("made presentation window:%p", (jobject)jWin);
	}
	else
	{
		JNI::InstMethod<jobject(jlong)> jSetMainContentView(env, baseActivity, "setMainContentView", "(J)Landroid/view/Window;");
		jWin = {env, jSetMainContentView(env, baseActivity, (jlong)this)};
		type = Type::MAIN;
		logMsg("made device window:%p", (jobject)jWin);
	}
	nPixelFormat = config.format() ? config.format() : toAHardwareBufferFormat(ctx.defaultWindowPixelFormat());
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
		if(type == Type::PRESENTATION)
		{
			logMsg("dismissing presentation window:%p", (jobject)jWin);
			jPresentationDeinit(jWin.jniEnv(), jWin);
		}
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
	surfaceChangeFlags |= WindowSurfaceChange::CONTENT_RECT_RESIZED;
}

bool Window::operator ==(Window const &rhs) const
{
	return jWin == rhs.jWin;
}

AndroidWindow::operator bool() const
{
	return jWin;
}

void AndroidWindow::setNativeWindow(ApplicationContext ctx, ANativeWindow *nWindow)
{
	auto &thisWindow = *static_cast<Window*>(this);
	if(nWin)
	{
		nWin = nullptr;
		thisWindow.dispatchSurfaceDestroyed();
	}
	if(!nWindow)
		return;
	nWin = nWindow;
	thisWindow.setFormat(nPixelFormat);
	if(onInit)
	{
		onInit(ctx, thisWindow);
		onInit = {};
	}
	else
	{
		thisWindow.dispatchSurfaceCreated();
	}
}

NativeWindow Window::nativeObject() const
{
	return nWin;
}

void Window::setIntendedFrameRate(double rate)
{
	if(appContext().androidSDK() < 30)
	{
		screen()->setFrameRate(rate);
		return;
	}
	if(!nWin) [[unlikely]]
		return;
	if(!ANativeWindow_setFrameRate) [[unlikely]]
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
	nPixelFormat = fmt;
	if(!nWin)
		return;
	if(appContext().androidSDK() < 11)
	{
		// In testing with CM7 on a Droid, not setting window format to match
		// what's used in ANativeWindow_setBuffersGeometry() may cause performance issues
		auto env = appContext().mainThreadJniEnv();
		if(Config::DEBUG_BUILD)
		{
			JNI::InstMethod<jobject()> jWinAttrs{env, (jobject)jWin, "getAttributes", "()Landroid/view/WindowManager$LayoutParams;"};
			auto attrs = jWinAttrs(env, jWin);
			jclass jLayoutParamsCls = env->GetObjectClass(attrs);
			auto jFormat = env->GetFieldID(jLayoutParamsCls, "format", "I");
			auto currFmt = env->GetIntField(attrs, jFormat);
			logMsg("setting window format:%s -> %s",
				aHardwareBufferFormatStr(currFmt), aHardwareBufferFormatStr(fmt));
		}
		JNI::InstMethod<void(jint)> jSetWinFormat{env, (jobject)jWin, "setFormat", "(I)V"};
		jSetWinFormat(env, jWin, fmt);
	}
	if(Config::DEBUG_BUILD)
		logMsg("setting window buffer format:%s -> %s",
			aHardwareBufferFormatStr(ANativeWindow_getFormat(nWin)), aHardwareBufferFormatStr(fmt));
	ANativeWindow_setBuffersGeometry(nWin, 0, 0, fmt);
}

void Window::setFormat(IG::PixelFormat fmt)
{
	setFormat(toAHardwareBufferFormat(fmt));
}

IG::PixelFormat Window::pixelFormat() const
{
	return makePixelFormatFromAndroidFormat(nPixelFormat);
}

int AndroidWindow::nativePixelFormat()
{
	return nPixelFormat;
}

void AndroidWindow::systemRequestsRedraw(bool sync)
{
	auto &win = *static_cast<Window*>(this);
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
	win.updateSize(winSize);
	win.postDraw();
}

void Window::setTitle(const char *name) {}

void Window::setAcceptDnd(bool on) {}

void WindowConfig::setFormat(IG::PixelFormat fmt)
{
	setFormat(toAHardwareBufferFormat(fmt));
}

}
