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

#define thisModuleName "base:android"
#include <stdlib.h>
#include <errno.h>
#include <util/egl.hh>
#include <logger/interface.h>
#include <engine-globals.h>
#include <base/android/sdk.hh>
#include <base/Base.hh>
#define USES_POLL_WAIT_TIMER
#include <base/common/funcs.h>
#include <input/android/private.hh>
#include <android/window.h>
#include <dlfcn.h>
#include <util/linux/uevent.h>

#include "private.hh"

#ifdef CONFIG_RESOURCE_FONT_ANDROID
	void setupResourceFontAndroidJni(JNIEnv *jEnv, jobject jClsLoader, const JavaInstMethod<jobject> &jLoadClass);
#endif

void* android_app_entry(void* param);

namespace Gfx
{
extern void setupAndroidOGLExtensions(const char *extensions, const char *rendererName);

AndroidSurfaceTextureConfig surfaceTextureConf;

void AndroidSurfaceTextureConfig::init(JNIEnv *jEnv)
{
	if(Base::androidSDK() >= 14)
	{
		logMsg("setting up SurfaceTexture JNI");
		// Surface members
		jSurfaceCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("android/view/Surface"));
		jSurface.setup(jEnv, jSurfaceCls, "<init>", "(Landroid/graphics/SurfaceTexture;)V");
		jSurfaceRelease.setup(jEnv, jSurfaceCls, "release", "()V");
		// SurfaceTexture members
		jSurfaceTextureCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("android/graphics/SurfaceTexture"));
		jSurfaceTexture.setup(jEnv, jSurfaceTextureCls, "<init>", "(I)V");
		//jSetDefaultBufferSize.setup(jEnv, jSurfaceTextureCls, "setDefaultBufferSize", "(II)V");
		jUpdateTexImage.setup(jEnv, jSurfaceTextureCls, "updateTexImage", "()V");
		jSurfaceTextureRelease.setup(jEnv, jSurfaceTextureCls, "release", "()V");
		use = 1;
	}
}

void AndroidSurfaceTextureConfig::deinit()
{
	// TODO
	jSurfaceTextureCls = nullptr;
	use = 0;
}

bool supportsAndroidSurfaceTexture() { return surfaceTextureConf.isSupported(); }
bool useAndroidSurfaceTexture() { return surfaceTextureConf.isSupported() ? surfaceTextureConf.use : 0; };
void setUseAndroidSurfaceTexture(bool on)
{
	if(surfaceTextureConf.isSupported())
		surfaceTextureConf.use = on;
}

}

namespace Base
{

struct EGLWindow
{
	EGLDisplay display = EGL_NO_DISPLAY;
	EGLSurface surface = EGL_NO_SURFACE;
	EGLContext context = EGL_NO_CONTEXT;
	EGLConfig config = 0;
	bool useMaxColorBits = 0;
	bool has32BppColorBugs = 0;

	constexpr EGLWindow() { }

	void initEGL()
	{
		logMsg("doing EGL init");
		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		assert(display != EGL_NO_DISPLAY);
		eglInitialize(display, 0, 0);

		//printEGLConfs(display);

		logMsg("%s (%s), extensions: %s", eglQueryString(display, EGL_VENDOR), eglQueryString(display, EGL_VERSION), eglQueryString(display, EGL_EXTENSIONS));
	}

	void initContext(ANativeWindow *win)
	{
		if(!useMaxColorBits)
		{
			logMsg("requesting lowest color config");
		}
		const EGLint *attribs = useMaxColorBits ? eglAttrWinMaxRGBA : eglAttrWinLowColor;
		EGLint configs = 0;
		eglChooseConfig(display, attribs, &config, 1, &configs);
		#ifndef NDEBUG
		printEGLConf(display, config);
		#endif

		assert(context == EGL_NO_CONTEXT);

		logMsg("creating GL context");
		context = eglCreateContext(display, config, 0, 0);

		initSurface(win);
	}

	static int winFormatFromConfig(EGLDisplay display, EGLConfig config)
	{
		EGLint nId;
		eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &nId);
		if(!nId)
		{
			nId = WINDOW_FORMAT_RGBA_8888;
			EGLint redSize;
			eglGetConfigAttrib(display, config, EGL_RED_SIZE, &redSize);
			if(redSize < 8)
				nId = WINDOW_FORMAT_RGB_565;
			logWarn("config didn't provide a native format id, guessing %d", nId);
		}
		return nId;
	}

	void initSurface(ANativeWindow *win)
	{
		assert(display != EGL_NO_DISPLAY);
		int configFormat = winFormatFromConfig(display, config);
		int currFormat = ANativeWindow_getFormat(win);
		if(currFormat != configFormat)
		{
			logMsg("changing window format from %d to %d", currFormat, configFormat);
			ANativeWindow_setBuffersGeometry(win, 0, 0, configFormat);
		}
		logMsg("current window format: %d", ANativeWindow_getFormat(win));

		assert(context != EGL_NO_CONTEXT);

		assert(surface == EGL_NO_SURFACE);
		logMsg("creating window surface");
		surface = eglCreateWindowSurface(display, config, win, 0);

		if(eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
		{
			logErr("error in eglMakeCurrent");
			abort();
		}
		logMsg("window size: %d,%d, from EGL: %d,%d", ANativeWindow_getWidth(win), ANativeWindow_getHeight(win), width(), height());

		/*if(eglSwapInterval(display, 1) != EGL_TRUE)
		{
			logErr("error in eglSwapInterval");
		}*/
	}

	EGLint width()
	{
		assert(surface != EGL_NO_SURFACE);
		assert(display != EGL_NO_DISPLAY);
		EGLint w;
		eglQuerySurface(display, surface, EGL_WIDTH, &w);
		return w;
	}

	EGLint height()
	{
		assert(surface != EGL_NO_SURFACE);
		assert(display != EGL_NO_DISPLAY);
		EGLint h;
		eglQuerySurface(display, surface, EGL_HEIGHT, &h);
		return h;
	}

	void destroySurface()
	{
		if(isDrawable())
		{
			logMsg("destroying window surface");
			eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglDestroySurface(display, surface);
			surface = EGL_NO_SURFACE;
		}
	}

	void destroyContext()
	{
		destroySurface();
		logMsg("destroying GL context");
		assert(surface == EGL_NO_SURFACE);
		assert(context != EGL_NO_CONTEXT);
		eglDestroyContext(display, context);
		context = EGL_NO_CONTEXT;
	}

	bool verifyContext()
	{
		return eglGetCurrentContext() != EGL_NO_CONTEXT;
	}

	bool isDrawable()
	{
		return surface != EGL_NO_SURFACE;
	}

	void swap()
	{
		eglSwapBuffers(display, surface);
	}
} eglWin;

}

#include "common.hh"

namespace Base
{

static JNIEnv* eJEnv = nullptr;
static bool engineIsInit = 0;
static JavaInstMethod<jint> jGetRotation;
static jobject jDpy;
JavaInstMethod<void> postUIThread;
static PollWaitTimer timerCallback;
static bool aHasFocus = 1;
JavaVM* jVM = 0;
extern pid_t activityTid;
static bool sigMatchesAPK = 1;
static jfieldID jSurfaceIs32BitId;

// Public implementation

JNIEnv* eEnv() { assert(eJEnv); return eJEnv; }
JNIEnv* aEnv() { return appInstance()->activity->env; }

uint appState = APP_PAUSED;

void setWindowPixelBestColorHint(bool best)
{
	assert(!engineIsInit); // should only call before initial window is created
	eglWin.useMaxColorBits = best;
}

bool windowPixelBestColorHintDefault()
{
	return Base::androidSDK() >= 11 && !eglWin.has32BppColorBugs;
}

void sendMessageToMain(int type, int shortArg, int intArg, int intArg2)
{
	assert(appInstance()->msgwrite);
	uint16 shortArg16 = shortArg;
	int msg[3] = { (shortArg16 << 16) | type, intArg, intArg2 };
	logMsg("sending msg type %d with args %d %d %d", msg[0] & 0xFFFF, msg[0] >> 16, msg[1], msg[2]);
	if(::write(appInstance()->msgwrite, &msg, sizeof(msg)) != sizeof(msg))
	{
		logErr("unable to write message to pipe: %s", strerror(errno));
	}
}

void sendMessageToMain(ThreadPThread &, int type, int shortArg, int intArg, int intArg2)
{
	sendMessageToMain(type, shortArg, intArg, intArg2);
}

#ifdef CONFIG_ANDROIDBT
static const ushort MSG_BT_DATA = 150;

void sendBTSocketData(BluetoothSocket &socket, int len, jbyte *data)
{
	int msg[3] = { MSG_BT_DATA, (int)&socket, len };
	if(::write(appInstance()->msgwrite, &msg, sizeof(msg)) != sizeof(msg))
	{
		logErr("unable to write message header to pipe: %s", strerror(errno));
	}
	if(::write(appInstance()->msgwrite, data, len) != len)
	{
		logErr("unable to write bt data to pipe: %s", strerror(errno));
	}
}
#endif

void sendTextEntryEnded(const char *str, jstring jStr)
{
	int msg[3] = { APP_CMD_TEXT_ENTRY_ENDED, (int)str, (int)jStr };
	if(::write(appInstance()->msgwrite, &msg, sizeof(msg)) != sizeof(msg))
	{
		logErr("unable to write text input message to pipe: %s", strerror(errno));
	}
}

void setIdleDisplayPowerSave(bool on)
{
	jint keepOn = !on;
	logMsg("keep screen on: %d", keepOn);
	postUIThread(eEnv(), jBaseActivity, 0, keepOn);
}

void setOSNavigationStyle(uint flags)
{
	// Flags mapped directly
	// OS_NAV_STYLE_DIM -> SYSTEM_UI_FLAG_LOW_PROFILE (1)
	// OS_NAV_STYLE_HIDDEN -> SYSTEM_UI_FLAG_HIDE_NAVIGATION (2)
	// 0 -> SYSTEM_UI_FLAG_VISIBLE
	postUIThread(eEnv(), jBaseActivity, 1, flags);
}

void addPollEvent2(int fd, PollEventDelegate &handler, uint events)
{
	logMsg("adding fd %d to looper", fd);
	assert(appInstance()->looper);
	int ret = ALooper_addFd(appInstance()->looper, fd, LOOPER_ID_USER, events, 0, &handler);
	assert(ret == 1);
}

void modPollEvent(int fd, PollEventDelegate &handler, uint events)
{
	addPollEvent2(fd, handler, events);
}

void removePollEvent(int fd)
{
	logMsg("removing fd %d from looper", fd);
	int ret = ALooper_removeFd(appInstance()->looper, fd);
	assert(ret != -1);
}

void openGLUpdateScreen()
{
	/*TimeSys preTime;
	preTime.setTimeNow();*/
	/*logMsg("swap buffers, surface %d %d, context %d, display %d", (int)eglGetCurrentSurface(EGL_DRAW), (int)eglGetCurrentSurface(EGL_READ),
			(int)eglGetCurrentContext(), (int)eglGetCurrentDisplay());*/
	eglWin.swap();
	/*TimeSys postTime;
	postTime.setTimeNow();
	logMsg("swap took %f", double(postTime - preTime));*/
}

bool surfaceTextureSupported()
{
	return Gfx::surfaceTextureConf.isSupported();
}

void setProcessPriority(int nice)
{
	assert(nice > -20);
	logMsg("setting process nice level: %d", nice);
	setpriority(PRIO_PROCESS, 0, nice);
	setpriority(PRIO_PROCESS, activityTid, nice == 0 ? 0 : nice-1);
}

int processPriority()
{
	return getpriority(PRIO_PROCESS, 0);
}

bool apkSignatureIsConsistent()
{
	return sigMatchesAPK;
}

// Private implementation

EGLDisplay getAndroidEGLDisplay()
{
	assert(eglWin.display != EGL_NO_DISPLAY);
	return eglWin.display;
}

bool windowIsDrawable()
{
	return eglWin.isDrawable();
}

// runs from activity thread, do not use jEnv
static void JNICALL jEnvConfig(JNIEnv* env, jobject thiz, jfloat xdpi, jfloat ydpi, jint refreshRate, jobject dpy,
		jstring devName, jstring filesPath, jstring eStoragePath, jstring apkPath, jobject sysVibrator,
		jboolean hasPermanentMenuKey, jboolean animatesRotation, jint sigHash)
{
	logMsg("doing java env config");
	logMsg("set screen DPI size %f,%f", (double)xdpi, (double)ydpi);
	// DPI values must come in un-rotated from DisplayMetrics
	androidXDPI = xDPI = xdpi;
	androidYDPI = yDPI = ydpi;

	jDpy = env->NewGlobalRef(dpy);
	refreshRate_ = refreshRate;
	logMsg("refresh rate: %d", refreshRate);

	const char *devNameStr = env->GetStringUTFChars(devName, 0);
	logMsg("device name: %s", devNameStr);
	setDeviceType(devNameStr);
	env->ReleaseStringUTFChars(devName, devNameStr);

	filesDir = env->GetStringUTFChars(filesPath, 0);
	eStoreDir = env->GetStringUTFChars(eStoragePath, 0);
	appPath = env->GetStringUTFChars(apkPath, 0);
	logMsg("apk @ %s", appPath);

	if(sysVibrator)
	{
		logMsg("Vibrator object present");
		vibrator = env->NewGlobalRef(sysVibrator);
		setupVibration(env);
	}

	Base::hasPermanentMenuKey = hasPermanentMenuKey;
	if(!hasPermanentMenuKey)
	{
		logMsg("device has software nav buttons");
	}

	osAnimatesRotation = animatesRotation;
	if(!osAnimatesRotation)
	{
		logMsg("app handles rotation animations");
	}

	if(Base::androidSDK() >= 11)
	{
		logMsg("defaulting to highest color mode");
		eglWin.useMaxColorBits = 1;
	}

	#ifdef ANDROID_APK_SIGNATURE_HASH
		sigMatchesAPK = sigHash == ANDROID_APK_SIGNATURE_HASH;
	#endif

	auto act = appInstance();
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&act->thread, &attr, android_app_entry, act);

	// Wait for thread to start.
	pthread_mutex_lock(&act->mutex);
	while (!act->running)
	{
		pthread_cond_wait(&act->cond, &act->mutex);
	}
	pthread_mutex_unlock(&act->mutex);
}

static void envConfig(int orientation, int hardKeyboardState, int navigationState)
{
	logMsg("doing native env config, starting orientation %d", orientation);
	osOrientation = orientation;

	aHardKeyboardState = (devType == DEV_TYPE_XPERIA_PLAY) ? navigationState : hardKeyboardState;
	logMsg("keyboard/nav hidden: %s", hardKeyboardNavStateToStr(aHardKeyboardState));
}

static void initGfxContext(struct android_app* app)
{
	auto prevUseMaxRGBBits = eglWin.useMaxColorBits;
	// Init OpenGL early so onInit() can set Android-specific bits,
	// and for detecting GPUs that need to default in 16-bit color
	eglWin.initEGL();
	eglWin.initContext(app->window);
	{
		auto extensions = (const char*)glGetString(GL_EXTENSIONS);
		assert(extensions);
		auto rendererName = (const char*)glGetString(GL_RENDERER);
		// Hack for Adreno chips that have display artifacts when using 32-bit color.
		if(Base::androidSDK() >= 11 && strstr(rendererName, "Adreno"))
		{
			logMsg("device may have broken 32-bit surfaces, defaulting to 16-bit");
			eglWin.useMaxColorBits = 0;
			eglWin.has32BppColorBugs = 1;
		}
		Gfx::setupAndroidOGLExtensions(extensions, rendererName);
	}
	doOrExit(onInit());
	if(prevUseMaxRGBBits != eglWin.useMaxColorBits)
	{
		// reinit context to handle changed EGL config
		eglWin.destroyContext();
		eglWin.initContext(app->window);
		eEnv()->SetBooleanField(jBaseActivity, jSurfaceIs32BitId, eglWin.useMaxColorBits);
	}
	initialScreenSizeSetup(ANativeWindow_getWidth(app->window), ANativeWindow_getHeight(app->window));
	engineInit();
	logMsg("done init");
	engineIsInit = 1;
}

static bool appFocus(bool hasFocus)
{
	aHasFocus = hasFocus;
	logMsg("focus change: %d", (int)hasFocus);
	auto prevGfxUpdateState = gfxUpdate;
	if(engineIsInit)
		onFocusChange(hasFocus);
	return appState == APP_RUNNING && prevGfxUpdateState == 0 && gfxUpdate;
}

static void configChange(struct android_app* app, jint hardKeyboardState, jint navState, jint orientation)
{
	logMsg("config change, keyboard: %s, navigation: %s", hardKeyboardNavStateToStr(hardKeyboardState), hardKeyboardNavStateToStr(navState));
	setHardKeyboardState((devType == DEV_TYPE_XPERIA_PLAY) ? navState : hardKeyboardState);

	if(setOrientationOS(orientation) && androidSDK() < 11)
	{
		logMsg("doing extra screen updates for orientation change");
		// hack for some Android 2.3 devices that won't return the correct window size
		// in the next redraw event unless a certain amount of frames are rendered
		if(appState == APP_RUNNING && eglWin.isDrawable())
		{
			iterateTimes(3, i)
			{
				int w = ANativeWindow_getWidth(app->window);
				int h = ANativeWindow_getHeight(app->window);
				mainWin.w = w; mainWin.h = h;
				resizeEvent(mainWin);
				gfxUpdate = 1;
				runEngine();
			}
		}
	}
}

static void appPaused()
{
	logMsg("app paused");
	appState = APP_PAUSED;
	if(engineIsInit)
	{
		onExit(1);
	}
}

static void appResumed()
{
	logMsg("app resumed");
	appState = APP_RUNNING;
	if(engineIsInit)
	{
		onResume(aHasFocus);
		displayNeedsUpdate();
	}
}

static bool statusBarIsHidden = 1; // assume app starts fullscreen

void setStatusBarHidden(uint hidden)
{
	hidden = hidden ? 1 : 0;
	if(hidden != statusBarIsHidden)
	{
		statusBarIsHidden = hidden;
		// changing window full-screen state may cause the window to re-create
		// so we need to queue it up for later in case other
		// window messages haven't been handled yet
		int msg[1] = { APP_CMD_TOGGLE_FULL_SCREEN_WINDOW };
		if(::write(appInstance()->msgwrite, &msg, sizeof(msg)) != sizeof(msg))
		{
			logErr("unable to write toggle full screen message to pipe: %s", strerror(errno));
		}
	}
}

static bool updateWinSize(Window &win, ANativeWindow* nWin)
{
	auto w = ANativeWindow_getWidth(nWin);
	auto h = ANativeWindow_getHeight(nWin);
	if(w < 0 || h < 0)
	{
		logMsg("error getting native window size");
		return 0;
	}
	win.w = w;
	win.h = h;
	return 1;
}

void onAppCmd(struct android_app* app, uint32 cmd)
{
	uint cmdType = cmd & 0xFFFF;
	switch (cmdType)
	{
		bcase APP_CMD_START:
		logMsg("app started");
		bcase APP_CMD_SAVE_STATE:
		//logMsg("got APP_CMD_SAVE_STATE");
		/*app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)app->savedState) = engine->state;
		app->savedStateSize = sizeof(struct saved_state);*/
		bcase APP_CMD_INIT_WINDOW:
		if(app->window)
		{
			logMsg("doing window init, size %d,%d", ANativeWindow_getWidth(app->window), ANativeWindow_getHeight(app->window));
			if(!engineIsInit)
			{
				initGfxContext(app);
			}
			else
			{
				eglWin.initSurface(app->window);
				updateWinSize(mainWin, app->window);
				resizeEvent(mainWin);
			}
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}
		bcase APP_CMD_TERM_WINDOW:
		logMsg("window destroyed");
		pthread_mutex_lock(&app->mutex);
		eglWin.destroySurface();
		pthread_cond_signal(&app->cond);
		pthread_mutex_unlock(&app->mutex);
		bcase APP_CMD_GAINED_FOCUS:
		if(app->window)
		{
			logMsg("app in focus, window size %d,%d", ANativeWindow_getWidth(app->window), ANativeWindow_getHeight(app->window));
			gfxUpdate = 1;
		}
		appFocus(1);
		bcase APP_CMD_LOST_FOCUS:
		appFocus(0);
		bcase APP_CMD_WINDOW_REDRAW_NEEDED:
		{
			if(eglWin.isDrawable())
			{
				// re-check if window size changed since some devices may "forget"
				// to send a proper resize event. For example, on a stock 2.3 Xperia Play,
				// putting the device to sleep in portrait, then sliding the gamepad open
				// causes a content rect change with swapped window width/height
				if(!updateWinSize(mainWin, app->window))
				{
					logWarn("skipping window redraw");
					break;
				}
				logMsg("window redraw, size %d,%d", mainWin.w, mainWin.h);
				resizeEvent(mainWin);
				gfxUpdate = 1;
				runEngine();
			}
			else
			{
				logWarn("window redraw without valid surface");
			}
		}
		bcase APP_CMD_WINDOW_RESIZED:
		{
			if(eglWin.isDrawable())
			{
				if(!updateWinSize(mainWin, app->window))
				{
					logWarn("skipping window resize");
					break;
				}
				logMsg("window resize to %d,%d", mainWin.w, mainWin.h);
				resizeEvent(mainWin);
				gfxUpdate = 1;
				runEngine();
			}
			else
			{
				logWarn("window resize without valid surface");
			}
		}
		bcase APP_CMD_CONTENT_RECT_CHANGED:
		{
			auto &rect = app->contentRect;
			mainWin.rect.x = rect.left; mainWin.rect.y = rect.top;
			mainWin.rect.x2 = rect.right; mainWin.rect.y2 = rect.bottom;
			if(eglWin.isDrawable())
			{
				if(updateWinSize(mainWin, app->window))
				{
					resizeEvent(mainWin);
					gfxUpdate = 1;
					runEngine();
					gfxUpdate = 1;
				}
			}
			logMsg("content rect changed to %d:%d:%d:%d, window size %d,%d%s",
				rect.left, rect.top, rect.right, rect.bottom, mainWin.w, mainWin.h, eglWin.isDrawable() ? "" : ", no valid surface");
		}
		/*bcase APP_CMD_LAYOUT_CHANGED:
		{
			logMsg("layout change, visible bottom %d", visibleScreenY);
			mainWin.rect.y2 = visibleScreenY;
			resizeEvent(mainWin);
			if(appState == APP_RUNNING && eglWin.isDrawable())
			{
				gfxUpdate = 1;
				runEngine();
			}
		}*/
		bcase APP_CMD_TOGGLE_FULL_SCREEN_WINDOW:
		{
			logMsg("setting app window fullscreen: %d", (int)statusBarIsHidden);
			ANativeActivity_setWindowFlags(appInstance()->activity,
				statusBarIsHidden ? AWINDOW_FLAG_FULLSCREEN : 0,
				statusBarIsHidden ? 0 : AWINDOW_FLAG_FULLSCREEN);
			sleepMs(50); // OS BUG: setting flags may cause instant destruction of the window,
				// including in mid EGL swap. Depending on the device driver, this will either
				// be ignored, create visual artifacts, or crash the app.
				// There doesn't seem to be any reliable method to sync the completion of
				// setWindowFlags in the Activity thread and there's no way to know
				// whether a particular call will actually re-create the window.
				// Thus, the only "solution" at the moment is to sleep for couple frames
				// and hope the app will find out about any window destruction
				// before another EGL swap
		}
		bcase APP_CMD_CONFIG_CHANGED:
		configChange(app, AConfiguration_getKeysHidden(app->config),
				AConfiguration_getNavHidden(app->config),
				jGetRotation(eEnv(), jDpy));
		bcase APP_CMD_PAUSE:
		appPaused();
		bcase APP_CMD_RESUME:
		appResumed();
		bcase APP_CMD_STOP:
		logMsg("app stopped");
		bcase APP_CMD_DESTROY:
		logMsg("app destroyed");
		app->activity->vm->DetachCurrentThread();
		::exit(0);
		bcase APP_CMD_INPUT_CHANGED:
		bcase APP_CMD_TEXT_ENTRY_ENDED:
		{
			#ifdef CONFIG_INPUT
			const char *str;
			read(app->msgread, &str, sizeof(str));
			jstring jStr;
			read(app->msgread, &jStr, sizeof(jStr));
			Input::textInputEndedMsg(str, jStr);
			#endif
		}
		#ifdef CONFIG_ANDROIDBT
		bcase MSG_BT_DATA:
		{
			BluetoothSocket *s;
			read(app->msgread, &s, sizeof(s));
			int size;
			read(app->msgread, &size, sizeof(size));
			uchar buff[48];
			read(app->msgread, buff, size);
			s->onDataDelegate().invoke(buff, size);
		}
		#endif
		bdefault:
		if(cmdType >= MSG_START)
		{
			uint32 arg[2];
			read(app->msgread, arg, sizeof(arg));
			logMsg("got msg type %d with args %d %d %d", cmd & 0xFFFF, cmd >> 16, arg[0], arg[1]);
			Base::processAppMsg(cmdType, cmd >> 16, arg[0], arg[1]);
		}
		else
			logWarn("got unknown cmd %d", cmd);
		break;
	}
}

/*static void JNICALL layoutChange(JNIEnv* env, jobject thiz, jint height)
{
	assert(bottom >= 0);
	if(visibleScreenY == (uint)bottom)
		return;
	logMsg("layout change, view height %d", bottom);
	visibleScreenY = bottom;
	if(!engineIsInit)
		return;
	uint32 cmd = APP_CMD_LAYOUT_CHANGED;
	write(appInstance()->msgwrite, &cmd, sizeof(cmd));
}*/

void jniInit(JNIEnv *jEnv, jobject inst) // uses JNIEnv from Activity thread
{
	using namespace Base;
	logMsg("doing pre-app JNI setup");

	// get class loader instance from Activity
	jclass jNativeActivityCls = jEnv->FindClass("android/app/NativeActivity");
	assert(jNativeActivityCls);
	JavaInstMethod<jobject> jGetClassLoader;
	jGetClassLoader.setup(jEnv, jNativeActivityCls, "getClassLoader", "()Ljava/lang/ClassLoader;");
	//jmethodID jGetClassLoaderID = jEnv->GetMethodID(jNativeActivityCls, "getClassLoader", "()Ljava/lang/ClassLoader;");
	//assert(jGetClassLoaderID);
	jobject jClsLoader = jGetClassLoader(jEnv, inst);
	assert(jClsLoader);

	jclass jClsLoaderCls = jEnv->FindClass("java/lang/ClassLoader");
	assert(jClsLoaderCls);
	JavaInstMethod<jobject> jLoadClass;
	jLoadClass.setup(jEnv, jClsLoaderCls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	//jmethodID jLoadClassID = jEnv->GetMethodID(jClsLoaderCls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	//assert(jLoadClassID);

	// BaseActivity members
	{
		jstring baseActivityStr = jEnv->NewStringUTF("com/imagine/BaseActivity");
		jBaseActivityCls = (jclass)jEnv->NewGlobalRef(jLoadClass(jEnv, jClsLoader, baseActivityStr));
		jEnv->DeleteLocalRef(baseActivityStr);
		jSetRequestedOrientation.setup(jEnv, jBaseActivityCls, "setRequestedOrientation", "(I)V");
		jAddNotification.setup(jEnv, jBaseActivityCls, "addNotification", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
		jRemoveNotification.setup(jEnv, jBaseActivityCls, "removeNotification", "()V");
		postUIThread.setup(jEnv, jBaseActivityCls, "postUIThread", "(II)V");
		jSurfaceIs32BitId = jEnv->GetFieldID(jBaseActivityCls, "surfaceIs32Bit", "Z");
	}

	#ifdef CONFIG_RESOURCE_FONT_ANDROID
		setupResourceFontAndroidJni(jEnv, jClsLoader, jLoadClass);
	#endif

	Gfx::surfaceTextureConf.init(jEnv);

	// Display members
	jclass jDisplayCls = jEnv->FindClass("android/view/Display");
	jGetRotation.setup(jEnv, jDisplayCls, "getRotation", "()I");

	static JNINativeMethod activityMethods[] =
	{
	    {"jEnvConfig", "(FFILandroid/view/Display;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Landroid/os/Vibrator;ZZI)V", (void *)&Base::jEnvConfig},
	    //{"layoutChange", "(I)V", (void *)&Base::layoutChange},
	};

	jEnv->RegisterNatives(jBaseActivityCls, activityMethods, sizeofArray(activityMethods));
}

}

static void dlLoadFuncs()
{
	void *libandroid = 0;

	if(Base::androidSDK() < 12) // no functions from dlopen needed before Android 3.1 (SDK 12)
	{
		return;
	}

	if((libandroid = dlopen("/system/lib/libandroid.so", RTLD_LOCAL | RTLD_LAZY)) == 0)
	{
		logWarn("unable to dlopen libandroid.so");
		return;
	}

	#ifdef CONFIG_INPUT_ANDROID
	Input::dlLoadAndroidFuncs(libandroid);
	#endif
}


static Base::CallbackRef *inputRescanCallbackRef = nullptr;
void inputRescanCallback()
{
	Input::rescanDevices();
	inputRescanCallbackRef = nullptr;
}

static int ueventFd = 0;
int ueventFdHandler(int events)
{
	using namespace Base;
	static const uint UEVENT_BUFFER_SIZE = 1024;
	if(events == Base::POLLEV_IN)
	{
		bool inputChange = 0;
		char buf[UEVENT_BUFFER_SIZE];
		int size = 0;
		while((size = recv(ueventFd, buf, sizeof(buf), 0)) > 0)
		{
			//logMsg("uevent: %s", buf);
			// NOTE: could also use InputManager class on Android 4.1+
			if(strstr(buf, "add@") || strstr(buf, "remove@"))
			{
				if(strstr(buf, "/event"))
				{
					logMsg("uevent: %s", buf);
					inputChange = 1;
				}
			}
		}
		if(size == -1 && errno != EAGAIN)
		{
			logErr("uevent recv failed: %s", strerror(errno));
			return 1;
		}
		if(inputChange)
		{
			if(inputRescanCallbackRef)
				cancelCallback(inputRescanCallbackRef);
			inputRescanCallbackRef = callbackAfterDelay(CallbackDelegate::create<inputRescanCallback>(), 100);
		}
		//logMsg("done reading uevents");
	}
	return 1;
}

void android_main(struct android_app* state)
{
	using namespace Base;
	assert(!engineIsInit); // catch accidental activity restarts
	logMsg("started native thread");
	dlLoadFuncs();
	{
		auto act = state->activity;
		auto config = state->config;
		jVM = act->vm;
		jBaseActivity = act->clazz;
		if(act->vm->AttachCurrentThread(&eJEnv, 0) != 0)
		{
			bug_exit("error attaching jEnv to thread");
		}
		envConfig(jGetRotation(eEnv(), jDpy),
			AConfiguration_getKeysHidden(config), AConfiguration_getNavHidden(config));
	}

	if(Base::androidSDK() >= 12)
	{
		ueventFd = openUeventFd();
		if (ueventFd >= 0)
		{
			auto ueventPoll = PollEventDelegate::create<&ueventFdHandler>();
			addPollEvent2(ueventFd, ueventPoll);
		}
	}

	/*TimeSys realTime;
	realTime.setTimeNow();*/
	for(;;)
	{
		int ident, events, fd;
		PollEventDelegate* source;
		//logMsg("entering looper");
		while((ident=ALooper_pollAll(getPollTimeout(), &fd, &events, (void**)&source)) >= 0)
		{
			//logMsg("out of looper with event id %d", ident);
			switch(ident)
			{
				bcase LOOPER_ID_MAIN: process_cmd(state);
				bcase LOOPER_ID_INPUT: if(likely(engineIsInit)) process_input(state);
				bdefault: // LOOPER_ID_USER
					assert(source);
					source->invoke(events);
			}
		}
		if(ident == -4)
			logMsg("out of looper with error");

		PollWaitTimer::processCallbacks();

		if(!gfxUpdate)
			logMsg("out of event loop without gfxUpdate, returned %d", ident);

		if(unlikely(appState != APP_RUNNING || !eglWin.isDrawable()))
		{
			if(gfxUpdate)
				logMsg("app gfx update halted");
			gfxUpdate = 0; // halt screen updates
			if(!eglWin.isDrawable())
				logWarn("EGL surface not drawable");
		}
		else
		{
			/*TimeSys prevTime = realTime;
			realTime.setTimeNow();
			logMsg("%f since last screen update", double(realTime - prevTime));*/
			runEngine();
		}
	}
}
