#define thisModuleName "base:androidWindow"
#include <base/common/windowPrivate.hh>
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
extern JavaInstMethod<void> jSetWinFormat, jSetWinFlags;
extern JavaInstMethod<int> jWinFormat;
extern JavaInstMethod<void> jSetRequestedOrientation;
extern JavaInstMethod<void> jPostDrawWindow, jCancelDrawWindow;
extern EGLContextHelper eglCtx;
extern EGLDisplay display;
extern AInputQueue *inputQueue;
extern ALooper *aLooper;
extern int osOrientation;
extern bool resumeAppOnWindowInit;
static void (*didDrawWindowCallback)() = nullptr;
extern uint drawWinEventIdle;
extern int drawWinEventFd;
extern jobject drawWindowHelper;
extern bool inputQueueAttached;
extern bool aHasFocus;
static uint extraRedraws_ = 0;
static CallbackRef *resizeCallback = nullptr;

int processInputCallback(int fd, int events, void* data);
void onResume(ANativeActivity* activity);
void onPause(ANativeActivity* activity);

static void initialScreenSizeSetup(Window &win, uint w, uint h)
{
	win.xDPI = androidXDPI;
	win.yDPI = androidYDPI;
	// assume content rectangle equals window size
	win.contentRect.x2 = w;
	win.contentRect.y2 = h;
	win.updateSize(w, h);
	if(androidSDK() < 9 && unlikely(win.viewMMWidth_ > 9000)) // hack for Archos Tablets
	{
		logMsg("screen size over 9000! setting to something sane");
		androidXDPI = win.xDPI = 220;
		androidYDPI = win.yDPI = 220;
		win.updateSize(w, h);
	}
}

static bool setEGLWindowFormat(JNIEnv *env, jobject activity, int currFormat)
{
	int configFormat = eglCtx.currentWindowFormat(display);
	if(currFormat != configFormat)
	{
		logMsg("changing window format from %d to %d", currFormat, configFormat);
		jSetWinFormat(env, activity, configFormat);
		return true;
	}
	else
		logMsg("keeping window format: %d", currFormat);
	return false;
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
	calcPhysicalSize();
	setupScreenSize();
}

uint Window::setOrientation(uint o)
{
	using namespace Base;
	logMsg("requested orientation change to %s", Base::orientationToStr(o));
	int toSet = -1;
	switch(o)
	{
		bdefault: bug_branch("%d", o);
		bcase VIEW_ROTATE_AUTO: toSet = -1; // SCREEN_ORIENTATION_UNSPECIFIED
		bcase VIEW_ROTATE_0: toSet = 1; // SCREEN_ORIENTATION_PORTRAIT
		bcase VIEW_ROTATE_90: toSet = 0; // SCREEN_ORIENTATION_LANDSCAPE
		bcase VIEW_ROTATE_180: toSet = androidSDK() > 8 ? 9 : 1; // SCREEN_ORIENTATION_REVERSE_PORTRAIT
		bcase VIEW_ROTATE_270: toSet = androidSDK() > 8 ? 8 : 0; // SCREEN_ORIENTATION_REVERSE_LANDSCAPE
		bcase VIEW_ROTATE_90 | VIEW_ROTATE_270: toSet = androidSDK() > 8 ? 6 : 1; // SCREEN_ORIENTATION_SENSOR_LANDSCAPE
	}
	jSetRequestedOrientation(eEnv(), jBaseActivity, toSet);
	return 1;
}

uint Window::setValidOrientations(uint oMask, bool manageAutoOrientation)
{
	return setOrientation(oMask);
}

void Window::setPixelBestColorHint(bool best)
{
	assert(!eglCtx.isInit()); // should only call before initial window is created
	eglCtx.useMaxColorBits = best;
	eglCtx.chooseConfig(display);
	#ifndef NDEBUG
	printEGLConf(display, eglCtx.config);
	logMsg("config value: %p", eglCtx.config);
	#endif
	auto env = eEnv();
	if(Base::androidSDK() < 11)
		setEGLWindowFormat(env, jBaseActivity, jWinFormat(env, jBaseActivity));
}

bool Window::pixelBestColorHintDefault()
{
	return Base::androidSDK() >= 11 && !eglCtx.has32BppColorBugs;
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
	mainWin = this;
	return OK;
}

void Window::show()
{
	displayNeedsUpdate();
}

static void initGfxContext(ANativeWindow* win)
{
	Gfx::init();
	initialScreenSizeSetup(mainWindow(), ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));
	Gfx::setViewport(mainWindow());
	Gfx::setProjector(mainWindow());
}

static void runPendingInit(ANativeActivity* activity, ANativeWindow* nWin)
{
	if(mainWindow().isFirstInit())
	{
		appState = APP_RUNNING;
		initGfxContext(nWin);
		onWindowInit(mainWindow());
		if(inputQueue && !inputQueueAttached)
		{
			logMsg("attaching input queue");
			AInputQueue_attachLooper(inputQueue, aLooper, ALOOPER_POLL_CALLBACK, processInputCallback, inputQueue);
		}
		// the following handlers should only ever be called after the initial window init
		activity->callbacks->onResume = onResume;
		activity->callbacks->onPause = onPause;
		handleIntent(activity);
	}
	else if(resumeAppOnWindowInit)
	{
		logMsg("running delayed onResume handler");
		mainWindow().updateSize(ANativeWindow_getWidth(nWin), ANativeWindow_getHeight(nWin));
		mainWindow().postResize();
		doOnResume(activity);
		resumeAppOnWindowInit = false;
	}
}

void Window::calcPhysicalSize()
{
	assert(osOrientation != -1);
	assert(xDPI && yDPI);
	float xdpi = ASurface::isStraightOrientation(osOrientation) ? xDPI : yDPI;
	float ydpi = ASurface::isStraightOrientation(osOrientation) ? yDPI : xDPI;
	viewMMWidth_ = ((float)w / xdpi) * 25.4;
	viewMMHeight_ = ((float)h / ydpi) * 25.4;
	viewSMMWidth_ = (w / aDensityDPI) * 25.4;
	viewSMMHeight_ = (h / aDensityDPI) * 25.4;
	logMsg("calc display size %fx%f MM, scaled %fx%f MM", (double)viewMMWidth_, (double)viewMMHeight_, (double)viewSMMWidth_, (double)viewSMMHeight_);
	assert(viewMMWidth_ && viewMMHeight_);
}

void Window::updateSize(int width, int height)
{
	w = width;
	h = height;
	viewRect = contentRect;
	viewPixelWidth_ = viewRect.xSize();
	viewPixelHeight_ = viewRect.ySize();
	calcPhysicalSize();
}

IG::Rect2<int> Window::untransformedViewBounds() const
{
	return contentRect;
}

void AndroidWindow::initSurface(EGLDisplay display, EGLConfig config, ANativeWindow *win)
{
	assert(display != EGL_NO_DISPLAY);
	assert(eglCtx.context != EGL_NO_CONTEXT);
	assert(surface == EGL_NO_SURFACE);
	logMsg("creating surface with format %d for window with format: %d", eglCtx.currentWindowFormat(display), ANativeWindow_getFormat(win));
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

void finishWindowInit(ANativeActivity* activity, ANativeWindow *win, bool hasFocus)
{
	mainWindow().unpostDraw(); // wait for onNativeWindowRedrawNeeded()
	//if(!mainWindow().inResize) logMsg("stopping window updates for resize operation");
	mainWindow().inResize = true;
	if(Base::androidSDK() < 11)
		setEGLWindowFormat(activity->env, activity->clazz, ANativeWindow_getFormat(win));
	mainWindow().nWin = win;
	mainWindow().initSurface(display, eglCtx.config, win);
	if(mainWindow().isFirstInit())
	{
		logMsg("window created");
	}
	else
	{
		logMsg("window re-created, size %d,%d", ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void postDrawWindowCallback()
{
	mainWindow().updateSize(ANativeWindow_getWidth(mainWindow().nWin), ANativeWindow_getHeight(mainWindow().nWin));
	mainWindow().postResize();
	logMsg("posted extra window redraw (%d left), size %d,%d", extraRedraws_, mainWindow().w, mainWindow().h);
	extraRedraws_--;
	if(!extraRedraws_)
		didDrawWindowCallback = nullptr;
}

void setWindowRedrawResizeCallback(uint extraRedraws)
{
	assert(extraRedraws);
	extraRedraws_ = extraRedraws;
	didDrawWindowCallback = postDrawWindowCallback;
}

void windowNeedsRedraw(ANativeActivity* activity, ANativeWindow *nWin)
{
	cancelCallback(resizeCallback);
	resizeCallback = nullptr;
	logMsg("window needs redraw, size %d,%d", ANativeWindow_getWidth(nWin), ANativeWindow_getHeight(nWin));
	assert(mainWindow().isDrawable());
	Window &win = mainWindow();
	win.inResize = false;
	//win.drawPosted = false; // reset from previous displayNeedsUpdate() calls while inResize == true
	win.displayNeedsUpdate();
	// post an extra draw to avoid incorrect display if the window draws too early
	// in an OS orientation change
	setWindowRedrawResizeCallback(1);
	runPendingInit(activity, nWin);
}

void windowResized(ANativeActivity* activity, ANativeWindow *win)
{
	logMsg("window resized, size %d,%d", ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));
	assert(mainWindow().isDrawable());
	mainWindow().unpostDraw(); // wait for onNativeWindowRedrawNeeded()
	//if(!mainWindow().inResize) logMsg("stopping window updates for resize operation");
	mainWindow().inResize = true;
	if(!mainWindow().isFirstInit())
	{
		cancelCallback(resizeCallback);
		resizeCallback = callbackAfterDelay(
			[activity, win]()
			{
				logMsg("forcing window redraw after resize");
				resizeCallback = nullptr;
				windowNeedsRedraw(activity, win);
			}, 10);
	}
}

void contentRectChanged(ANativeActivity* activity, const ARect &rect, ANativeWindow *win)
{
	if(!mainWindow().nWin)
	{
		logMsg("ignoring content rect request of uninitialized window");
		return;
	}
	mainWindow().displayNeedsUpdate();
	setWindowRedrawResizeCallback(1);
	mainWindow().contentRect.x = rect.left; mainWindow().contentRect.y = rect.top;
	mainWindow().contentRect.x2 = rect.right; mainWindow().contentRect.y2 = rect.bottom;

	// Post an extra draw and delay applying the viewport to avoid incorrect display
	// if the window draws too early in an OS UI animation (navigation bar slide, etc.)
	// or the content rect is out of sync (happens mostly on pre-3.0 OS versions).
	// For example, on a stock 2.3 Xperia Play,
	// putting the device to sleep in portrait, then sliding the gamepad open
	// may cause a content rect change with swapped window width/height.
	// Another example, on the Archos Gamepad,
	// removing the navigation bar can make the viewport off center
	// if it's updated directly on the next frame.
	//didDrawWindowCallback = postDrawWindowCallback;
	logMsg("content rect changed to %d:%d:%d:%d, window size %d,%d",
		rect.left, rect.top, rect.right, rect.bottom, mainWindow().w, mainWindow().h);
	runPendingInit(activity, win);
}

bool drawWindow(int64 frameTimeNanos)
{
	if(!mainWindow().drawPosted)
	{
		logMsg("ignoring spurious graphics update");
		return false;
	}
	if(mainWindow().inResize)
	{
		bug_exit("window should be in unposted state during resize operation");
	}
	//logMsg("called drawWindow");

	// bypass regular post/un-post behavior in draw callback
	// and let return value of this function handle it
	mainWindow().inDraw = true;
	drawWindows(frameTimeNanos);
	if(unlikely(didDrawWindowCallback))
		didDrawWindowCallback();
	mainWindow().inDraw = false;
	if(mainWindow().drawPosted)
	{
		//logMsg("drawing next frame");
		return true;
	}
	else
	{
		//logMsg("stopping with this frame");
		return false;
	}
}

void Window::displayNeedsUpdate()
{
	if(unlikely(!nWin))
	{
		logMsg("cannot post redraw to uninitialized window");
		return;
	}
	if(unlikely(inResize))
	{
		logMsg("cannot post redraw due to resize operation");
		return;
	}
	else if(!drawPosted && inDraw)
	{
		//logMsg("posting window draw while in draw operation");
		drawPosted = true;
	}
	else if(!drawPosted)
	{
		//logMsg("posting window draw");
		drawPosted = true;
		if(jPostDrawWindow.m)
			jPostDrawWindow(eEnv(), drawWindowHelper);
		else
		{
			uint64_t post = 1;
			auto ret = write(drawWinEventFd, &post, sizeof(post));
			assert(ret == sizeof(post));
		}
	}
}

void Window::unpostDraw()
{
	if(drawPosted && inDraw)
	{
		logMsg("un-posting window draw while in draw operation");
		drawPosted = false;
	}
	else if(drawPosted)
	{
		logMsg("un-posting window draw");
		drawPosted = false;
		if(jCancelDrawWindow.m)
			jCancelDrawWindow(eEnv(), drawWindowHelper);
		else
		{
			uint64_t post;
			read(drawWinEventFd, &post, sizeof(post));
			drawWinEventIdle = 1; // force handler to idle since it could already be signaled by epoll
		}
	}
}

void restoreOpenGLContext()
{
	eglCtx.restore(display, mainWindow().surface);
}

void windowDestroyed(ANativeActivity* activity, Window &win)
{
	if(win.isDrawable())
	{
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		logMsg("destroying window surface");
		eglDestroySurface(display, win.surface);
		win.surface = EGL_NO_SURFACE;
	}
	cancelCallback(resizeCallback);
	resizeCallback = nullptr;
	win.nWin = nullptr;
	win.unpostDraw();
	/*if(Base::androidSDK() < 11)
	{
		// In testing with CM7 on a Droid, the surface is re-created in RGBA8888 upon
		// resuming the app and ANativeWindow_setBuffersGeometry() has no effect.
		// Explicitly setting the format here seems to fix the problem. Android bug?
		logMsg("explicitly setting destroyed window format to %d", eglCtx.currentWindowFormat(display));
		jSetWinFormat(activity->env, activity->clazz, eglCtx.currentWindowFormat(display));
	}*/
}

}
