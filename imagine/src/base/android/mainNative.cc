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
#include <cstdlib>
#include <errno.h>

#include <logger/interface.h>
#include <engine-globals.h>
#include <base/android/sdk.hh>
#include <base/Base.hh>
#include <base/common/funcs.h>
#include <input/android/private.hh>
#include <android/window.h>
#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>
#include <dlfcn.h>
#include <sys/inotify.h>
#include <sys/eventfd.h>
#include <fs/sys.hh>
#include <util/fd-utils.h>
#ifdef CONFIG_BLUETOOTH
#include <bluetooth/BluetoothInputDevScanner.hh>
#endif
#include "private.hh"
#include "common.hh"
#include "EGLWindow.hh"

namespace Gfx
{

AndroidSurfaceTextureConfig surfaceTextureConf;

void AndroidSurfaceTextureConfig::init(JNIEnv *jEnv)
{
	if(Base::androidSDK() >= 14)
	{
		//logMsg("setting up SurfaceTexture JNI");
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
	use = whiteListed = 0;
}

bool supportsAndroidSurfaceTexture() { return surfaceTextureConf.isSupported(); }
bool supportsAndroidSurfaceTextureWhitelisted() { return surfaceTextureConf.isSupported() && surfaceTextureConf.whiteListed; };
bool useAndroidSurfaceTexture() { return surfaceTextureConf.isSupported() ? surfaceTextureConf.use : 0; };
void setUseAndroidSurfaceTexture(bool on)
{
	if(surfaceTextureConf.isSupported())
		surfaceTextureConf.use = on;
}

}

namespace Base
{

static void processInputWithGetEvent(AInputQueue *inputQueue);
static void processInputWithHasEvents(AInputQueue *inputQueue);
static void postDrawWindow();
static void cancelDrawWindow();
static int processInputCallback(int fd, int events, void* data);
static void onResume(ANativeActivity* activity);
static void onPause(ANativeActivity* activity);
static jboolean JNICALL drawWindowJNI(JNIEnv* env, jobject thiz, jlong frameTimeNanos);
static jboolean JNICALL drawWindowIdleJNI(JNIEnv* env, jobject thiz);
static int drawWinEventFdHandler(int fd, int events, void* data);

static JNIEnv* eJEnv = nullptr;
bool engineIsInit = 0;
static JavaInstMethod<jint> jGetRotation;
static jobject jDpy = nullptr;
static jobject drawWindowHelper = nullptr;
static bool aHasFocus = 1;
JavaVM* jVM = 0;
extern pid_t activityTid;
static bool sigMatchesAPK = 1;
static bool resumeAppOnWindowInit = 0;
static bool hasChoreographer = 0;
static AInputQueue *inputQueue = nullptr;
static ALooper *aLooper = nullptr;
static ANativeWindow *nWin = nullptr;
static int64 prevFrameTimeNanos = 0;
static EGLWindow eglWin;
uint appState = APP_PAUSED;
static AConfiguration *aConfig = nullptr;
static int writeMsgPipe = -1, drawWinEventFd = -1;
static JavaInstMethod<void> jSetKeepScreenOn, jSetUIVisibility, jSetFullscreen,
	jPostWinDraw, jCancelWinDraw, jPostDrawWindow, jCancelDrawWindow;
static JavaInstMethod<jfloat> jGetRefreshRate;
static bool winDrawPosted = 0;
static uint drawWinEventIdle = 0;
static void (*processInput)(AInputQueue *inputQueue) = Base::processInputWithHasEvents;
static bool statusBarIsHidden = 1; // assume app starts fullscreen
static CallbackRef *inputRescanCallbackRef = nullptr;
static void (*didDrawWindowCallback)() = nullptr;
static const char *buildDevice = nullptr;
static uint refreshRate_ = 0;
static JavaInstMethod<jobject> jNewFontRenderer;

const char *androidBuildDevice()
{
	assert(buildDevice);
	return buildDevice;
}

void setWindowPixelBestColorHint(bool best)
{
	assert(!engineIsInit); // should only call before initial window is created
	eglWin.useMaxColorBits = best;
}

bool windowPixelBestColorHintDefault()
{
	return Base::androidSDK() >= 11 && !eglWin.has32BppColorBugs;
}

uint refreshRate()
{
	if(!refreshRate_)
	{
		assert(jDpy);
		refreshRate_ = jGetRefreshRate(eEnv(), jDpy);
		logMsg("refresh rate: %d", refreshRate_);
	}
	return refreshRate_;
}

static int pollEventCallback(int fd, int events, void* data)
{
	auto source = *((PollEventDelegate*)data);
	source(events);
	if(gfxUpdate)
		postDrawWindow();
	return 1;
}

ALooper *activityLooper()
{
	assert(aLooper);
	return aLooper;
}

static void addPollEvent(ALooper *looper, int fd, PollEventDelegate &handler, uint events)
{
	logMsg("adding fd %d to looper", fd);
	assert(looper);
	int ret = ALooper_addFd(looper, fd, ALOOPER_POLL_CALLBACK, events, pollEventCallback, &handler);
	assert(ret == 1);
}

static void removePollEvent(ALooper *looper, int fd)
{
	logMsg("removing fd %d from looper", fd);
	int ret = ALooper_removeFd(looper, fd);
	assert(ret != -1);
}

void openGLUpdateScreen()
{
	eglWin.swap();
}

bool surfaceTextureSupported()
{
	return Gfx::surfaceTextureConf.isSupported();
}

int processPriority()
{
	return getpriority(PRIO_PROCESS, 0);
}

bool apkSignatureIsConsistent()
{
	return sigMatchesAPK;
}

EGLDisplay getAndroidEGLDisplay()
{
	assert(eglWin.display != EGL_NO_DISPLAY);
	return eglWin.display;
}

jobject newFontRenderer(JNIEnv *jEnv)
{
	return jNewFontRenderer(jEnv, jBaseActivity);
}

static void initConfig(AConfiguration* config)
{
	auto hardKeyboardState = AConfiguration_getKeysHidden(config);
	auto navigationState = AConfiguration_getNavHidden(config);
	auto keyboard = AConfiguration_getKeyboard(config);

	aHardKeyboardState = Input::hasXperiaPlayGamepad() ? navigationState : hardKeyboardState;
	logMsg("keyboard/nav hidden: %s", hardKeyboardNavStateToStr(aHardKeyboardState));

	aKeyboardType = keyboard;
	if(aKeyboardType != ACONFIGURATION_KEYBOARD_NOKEYS)
		logMsg("keyboard type: %d", aKeyboardType);
}

static void initGfxContext(ANativeWindow* win)
{
	eglWin.initEGL();
	eglWin.initContext(win);
	initialScreenSizeSetup(ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));
	engineInit();
	logMsg("done init");
	engineIsInit = 1;
}

static void appFocus(bool hasFocus, ANativeWindow* window)
{
	aHasFocus = hasFocus;
	logMsg("focus change: %d", (int)hasFocus);
	if(hasFocus && window)
	{
		logMsg("app in focus, window size %d,%d", ANativeWindow_getWidth(window), ANativeWindow_getHeight(window));
		gfxUpdate = 1;
	}
	if(engineIsInit)
		onFocusChange(hasFocus);
}

static void configChange(JNIEnv* jEnv, AConfiguration* config, ANativeWindow* window)
{
	auto hardKeyboardState = AConfiguration_getKeysHidden(config);
	auto navState = AConfiguration_getNavHidden(config);
	auto orientation = jGetRotation(jEnv, jDpy);
	auto keyboard = AConfiguration_getKeyboard(config);
	logMsg("config change, keyboard: %s, navigation: %s", hardKeyboardNavStateToStr(hardKeyboardState), hardKeyboardNavStateToStr(navState));
	setHardKeyboardState(Input::hasXperiaPlayGamepad() ? navState : hardKeyboardState);

	if(setOrientationOS(orientation))
	{
		//logMsg("changed OS orientation");
	}
}

static void appPaused()
{
	logMsg("app paused");
	appState = APP_PAUSED;
	onExit(1);
	#ifdef CONFIG_AUDIO
	Audio::updateFocusOnPause();
	#endif
}

static void appResumed()
{
	appState = APP_RUNNING;
	#ifdef CONFIG_AUDIO
	Audio::updateFocusOnResume();
	#endif
	if(eglWin.isDrawable())
	{
		logMsg("app resumed");
		onResume(aHasFocus);
		displayNeedsUpdate();
	}
	else
	{
		logMsg("app resumed without window, delaying onResume handler");
		resumeAppOnWindowInit = 1;
	}
}

static void updateWinSize(Window &win, ANativeWindow* nWin)
{
	auto w = ANativeWindow_getWidth(nWin);
	auto h = ANativeWindow_getHeight(nWin);
	bool changed = w != win.w || h != win.h;
	win.w = w;
	win.h = h;
	if(w < 0 || h < 0)
	{
		bug_exit("error getting native window size");
	}
	if(changed)
	{
		logMsg("set window size %d,%d", w, h);
	}
}

static int inputDevNotifyFdHandler(int fd, int events, void* data)
{
	logMsg("got inotify event");
	if(events == Base::POLLEV_IN)
	{
		char buffer[2048];
		auto size = read(fd, buffer, sizeof(buffer));
		if(inputRescanCallbackRef)
			cancelCallback(inputRescanCallbackRef);
		inputRescanCallbackRef = callbackAfterDelay(
			[]()
			{
				Input::rescanDevices();
				inputRescanCallbackRef = nullptr;
				postDrawWindowIfNeeded();
			},
			250);
	}
	return 1;
}

static void initWindow(ANativeActivity* activity, ANativeWindow *win)
{
	if(win)
	{
		if(!engineIsInit)
		{
			logMsg("doing window & gfx context init");
			appState = APP_RUNNING;
			initGfxContext(win);
			if(inputQueue)
			{
				logMsg("attaching input queue");
				AInputQueue_attachLooper(inputQueue, aLooper, ALOOPER_POLL_CALLBACK, processInputCallback, inputQueue);
			}
			// the following handlers should only ever be called after the initial window init
			activity->callbacks->onResume = onResume;
			activity->callbacks->onPause = onPause;
		}
		else
		{
			logMsg("doing window init, size %d,%d", ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));
			eglWin.initSurface(win);
			updateWinSize(mainWin, win);
			resizeEvent(mainWin);
			if(resumeAppOnWindowInit)
			{
				logMsg("running delayed onResume handler");
				onResume(aHasFocus);
				displayNeedsUpdate();
				resumeAppOnWindowInit = 0;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
}

static void postDrawWindowCallback()
{
	gfxUpdate = 1;
	postDrawWindow();
	// re-check if window size changed in case OS didn't sync properly with app
	updateWinSize(mainWin, nWin);
	resizeEvent(mainWin);
	triggerGfxResize = 1;
	logMsg("extra window redraw, size %d,%d", mainWin.w, mainWin.h);
	didDrawWindowCallback = nullptr;
}

static void windowNeedsRedraw(ANativeWindow *win)
{
	assert(eglWin.isDrawable());
	gfxUpdate = 1;
	postDrawWindow();
	// post an extra draw to avoid incorrect display if the window draws too early
	// in an OS orientation change
	didDrawWindowCallback = postDrawWindowCallback;
	logMsg("window redraw, size %d,%d", mainWin.w, mainWin.h);
}

static void windowResized(ANativeWindow *win)
{
	assert(eglWin.isDrawable());
	logMsg("window resized");
	gfxUpdate = 1;
	postDrawWindow();
	updateWinSize(mainWin, win);
	resizeEvent(mainWin);
}

static void contentRectChanged(const ARect &rect, ANativeWindow *win)
{
	mainWin.rect.x = rect.left; mainWin.rect.y = rect.top;
	mainWin.rect.x2 = rect.right; mainWin.rect.y2 = rect.bottom;
	assert(eglWin.isDrawable());
	gfxUpdate = 1;
	postDrawWindow();
	updateWinSize(mainWin, win);
	resizeEvent(mainWin);

	// Post an extra draw and delay applying the viewport to avoid incorrect display
	// if the window draws too early in an OS UI animation (navigation bar slide, etc.)
	// or the content rect is out of sync (happens mostly on pre-3.0 OS versions).
	// For example, on a stock 2.3 Xperia Play,
	// putting the device to sleep in portrait, then sliding the gamepad open
	// may cause a content rect change with swapped window width/height.
	// Another example, on the Archos Gamepad,
	// removing the navigation bar can make the viewport off center
	// if it's updated directly on the next frame.
	triggerGfxResize = 0;
	didDrawWindowCallback = postDrawWindowCallback;
	logMsg("content rect changed to %d:%d:%d:%d, window size %d,%d%s",
		rect.left, rect.top, rect.right, rect.bottom, mainWin.w, mainWin.h, eglWin.isDrawable() ? "" : ", no valid surface");
}

static void activityInit(ANativeActivity* activity) // uses JNIEnv from Activity thread
{
	auto jEnv = activity->env;
	auto inst = activity->clazz;
	using namespace Base;
	logMsg("doing app creation");

	// BaseActivity members
	{
		jBaseActivityCls = (jclass)jEnv->NewGlobalRef(jEnv->GetObjectClass(inst));
		jSetRequestedOrientation.setup(jEnv, jBaseActivityCls, "setRequestedOrientation", "(I)V");
		jAddNotification.setup(jEnv, jBaseActivityCls, "addNotification", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
		jRemoveNotification.setup(jEnv, jBaseActivityCls, "removeNotification", "()V");
		#ifdef CONFIG_RESOURCE_FONT_ANDROID
		jNewFontRenderer.setup(jEnv, jBaseActivityCls, "newFontRenderer", "()Lcom/imagine/FontRenderer;");
		#endif


		if(Base::androidSDK() < 11) // bug in pre-3.0 Android causes paths in ANativeActivity to be null
		{
			logMsg("ignoring paths from ANativeActivity due to Android 2.3 bug");
			JavaInstMethod<jobject> jFilesDir;
			jFilesDir.setup(jEnv, jBaseActivityCls, "filesDir", "()Ljava/lang/String;");
			filesDir = jEnv->GetStringUTFChars((jstring)jFilesDir(jEnv, inst), 0);
		}
		else
		{
			filesDir = activity->internalDataPath;
			//eStoreDir = activity->externalDataPath;
		}

		{
			JavaClassMethod<jobject> extStorageDir;
			extStorageDir.setup(jEnv, jBaseActivityCls, "extStorageDir", "()Ljava/lang/String;");
			eStoreDir = jEnv->GetStringUTFChars((jstring)extStorageDir(jEnv), 0);
			assert(filesDir);
			assert(eStoreDir);
			logMsg("internal storage path: %s", filesDir);
			logMsg("external storage path: %s", eStoreDir);
		}

		doOrExit(logger_init());
		logMsg("SDK API Level: %d", aSDK);

		{
			JavaInstMethod<jobject> jApkPath;
			jApkPath.setup(jEnv, jBaseActivityCls, "apkPath", "()Ljava/lang/String;");
			appPath = jEnv->GetStringUTFChars((jstring)jApkPath(jEnv, inst), 0);
			logMsg("apk @ %s", appPath);
		}

		{
			JavaClassMethod<jobject> jDevName;
			jDevName.setup(jEnv, jBaseActivityCls, "devName", "()Ljava/lang/String;");
			auto devName = (jstring)jDevName(jEnv);
			buildDevice = jEnv->GetStringUTFChars(devName, 0);
			logMsg("device name: %s", buildDevice);
			setDeviceType(buildDevice);
		}

		if(!Config::MACHINE_IS_OUYA)
		{
			JavaInstMethod<jobject> jSysVibrator;
			jSysVibrator.setup(jEnv, jBaseActivityCls, "systemVibrator", "()Landroid/os/Vibrator;");
			vibrator = jSysVibrator(jEnv, inst);
			if(vibrator)
			{
				logMsg("Vibrator present");
				vibrator = jEnv->NewGlobalRef(vibrator);
			}
		}

		if(Base::androidSDK() >= 11)
			osAnimatesRotation = 1;
		else
		{
			JavaClassMethod<jboolean> jAnimatesRotation;
			jAnimatesRotation.setup(jEnv, jBaseActivityCls, "gbAnimatesRotation", "()Z");
			osAnimatesRotation = jAnimatesRotation(jEnv);
		}
		if(!osAnimatesRotation)
		{
			logMsg("app handles rotation animations");
		}

		if(!Config::MACHINE_IS_OUYA)
		{
			if(Base::androidSDK() >= 14)
			{
				JavaInstMethod<jboolean> jHasPermanentMenuKey;
				jHasPermanentMenuKey.setup(jEnv, jBaseActivityCls, "hasPermanentMenuKey", "()Z");
				Base::hasPermanentMenuKey = jHasPermanentMenuKey(jEnv, inst);
				if(!Base::hasPermanentMenuKey)
				{
					logMsg("device has software nav buttons");
				}
			}
			else
				Base::hasPermanentMenuKey = 1;
		}

		#ifdef ANDROID_APK_SIGNATURE_HASH
			JavaInstMethod<jint> jSigHash;
			jSigHash.setup(jEnv, jBaseActivityCls, "sigHash", "()I");
			sigMatchesAPK = jSigHash(jEnv, inst) == ANDROID_APK_SIGNATURE_HASH;
		#endif
	}

	Gfx::surfaceTextureConf.init(jEnv);

	// Display
	JavaInstMethod<jobject> jDefaultDpy;
	jDefaultDpy.setup(jEnv, jBaseActivityCls, "defaultDpy", "()Landroid/view/Display;");
	jDpy = jEnv->NewGlobalRef(jDefaultDpy(jEnv, inst));

	jclass jDisplayCls = jEnv->GetObjectClass(jDpy);
	jGetRotation.setup(jEnv, jDisplayCls, "getRotation", "()I");
	jGetRefreshRate.setup(jEnv, jDisplayCls, "getRefreshRate", "()F");
	//JavaInstMethod<void> jGetMetrics;
	//jGetMetrics.setup(jEnv, jDisplayCls, "getMetrics", "(Landroid/util/DisplayMetrics;)V");

	auto orientation = jGetRotation(jEnv, jDpy);
	logMsg("starting orientation %d", orientation);
	osOrientation = orientation;
	bool isStraightOrientation = !Surface::isSidewaysOrientation(orientation);

	// DisplayMetrics
	JavaInstMethod<jobject> jDisplayMetrics;
	jDisplayMetrics.setup(jEnv, jBaseActivityCls, "displayMetrics", "()Landroid/util/DisplayMetrics;");
	// DisplayMetrics obtained via getResources().getDisplayMetrics() so the scaledDensity field is correct
	auto dpyMetrics = jDisplayMetrics(jEnv, inst);
	assert(dpyMetrics);

	jclass jDisplayMetricsCls = jEnv->GetObjectClass(dpyMetrics);
	//JavaInstMethod<void> jDisplayMetrics;
	//jDisplayMetrics.setup(jEnv, jDisplayMetricsCls, "<init>", "()V");
	auto jXDPI = jEnv->GetFieldID(jDisplayMetricsCls, "xdpi", "F");
	auto jYDPI = jEnv->GetFieldID(jDisplayMetricsCls, "ydpi", "F");
	auto jScaledDensity = jEnv->GetFieldID(jDisplayMetricsCls, "scaledDensity", "F");
	#ifndef NDEBUG
	{
		auto jDensity = jEnv->GetFieldID(jDisplayMetricsCls, "density", "F");
		auto jDensityDPI = jEnv->GetFieldID(jDisplayMetricsCls, "densityDpi", "I");
		auto jWidthPixels = jEnv->GetFieldID(jDisplayMetricsCls, "widthPixels", "I");
		auto jHeightPixels = jEnv->GetFieldID(jDisplayMetricsCls, "heightPixels", "I");
		logMsg("display density %f, densityDPI %d, %dx%d pixels",
			(double)jEnv->GetFloatField(dpyMetrics, jDensity), jEnv->GetIntField(dpyMetrics, jDensityDPI),
			jEnv->GetIntField(dpyMetrics, jWidthPixels), jEnv->GetIntField(dpyMetrics, jHeightPixels));
	}
	#endif

	auto metricsXDPI = jEnv->GetFloatField(dpyMetrics, jXDPI);
	auto metricsYDPI = jEnv->GetFloatField(dpyMetrics, jYDPI);
	aDensityDPI = 160.*jEnv->GetFloatField(dpyMetrics, jScaledDensity);
	assert(aDensityDPI);
	logMsg("set screen DPI size %f,%f, scaled density DPI %f", (double)metricsXDPI, (double)metricsYDPI, (double)aDensityDPI);
	if(Config::MACHINE_IS_GENERIC_ARMV7 && Base::androidSDK() >= 16 && (strstr(buildDevice, "spyder") || strstr(buildDevice, "targa")))
	{
		logMsg("using scaled DPI as physical DPI due to Droid RAZR/Bionic Android 4.1 bug");
		metricsXDPI = metricsYDPI = aDensityDPI;
	}
	// DPI values are un-rotated from DisplayMetrics
	androidXDPI = xDPI = isStraightOrientation ? metricsXDPI : metricsYDPI;
	androidYDPI = yDPI = isStraightOrientation ? metricsYDPI : metricsXDPI;

	if(Config::MACHINE_IS_GENERIC_ARMV7 && Base::androidSDK() >= 11)
	{
		if(FsSys::fileExists("/system/lib/egl/libEGL_adreno200.so"))
		{
			// Hack for Adreno chips that have display artifacts when using 32-bit color.
			logMsg("device may have broken 32-bit surfaces, defaulting to low color");
			eglWin.useMaxColorBits = 0;
			eglWin.has32BppColorBugs = 1;
		}
		else
		{
			logMsg("defaulting to highest color mode");
			eglWin.useMaxColorBits = 1;
		}
	}

	doOrExit(onInit(0, nullptr));

	jSetKeepScreenOn.setup(jEnv, jBaseActivityCls, "setKeepScreenOn", "(Z)V");
	jSetFullscreen.setup(jEnv, jBaseActivityCls, "setFullscreen", "(Z)V");
	jSetUIVisibility.setup(jEnv, jBaseActivityCls, "setUIVisibility", "(I)V");
	if(Base::androidSDK() >= 16)
	{
		//logMsg("using Choreographer for display updates");
		hasChoreographer = 1;
		JavaInstMethod<jobject> jNewChoreographerHelper;
		jNewChoreographerHelper.setup(jEnv, jBaseActivityCls, "newChoreographerHelper", "()Lcom/imagine/ChoreographerHelper;");
		drawWindowHelper = jNewChoreographerHelper(jEnv, inst);
		assert(drawWindowHelper);
		drawWindowHelper = jEnv->NewGlobalRef(drawWindowHelper);
		auto choreographerHelperCls = jEnv->GetObjectClass(drawWindowHelper);
		jPostDrawWindow.setup(jEnv, choreographerHelperCls, "postDrawWindow", "()V");
		jCancelDrawWindow.setup(jEnv, choreographerHelperCls, "cancelDrawWindow", "()V");
		{
			JNINativeMethod method[] =
			{
					{"drawWindow", "(J)Z", (void*)&Base::drawWindowJNI},
			};
			jEnv->RegisterNatives(choreographerHelperCls, method, sizeofArray(method));
		}
	}
	else
	{
		drawWinEventFd = eventfd(0, 0);
		if(unlikely(drawWinEventFd == -1))
		{
			logWarn("error creating eventfd: %d (%s), falling back to idle handler", errno, strerror(errno));
			JavaInstMethod<jobject> jNewIdleHelper;
			jNewIdleHelper.setup(jEnv, jBaseActivityCls, "newIdleHelper", "()Lcom/imagine/BaseActivity$IdleHelper;");
			drawWindowHelper = jNewIdleHelper(jEnv, inst);
			assert(drawWindowHelper);
			drawWindowHelper = jEnv->NewGlobalRef(drawWindowHelper);
			auto idleHelperCls = jEnv->GetObjectClass(drawWindowHelper);
			jPostDrawWindow.setup(jEnv, idleHelperCls, "postDrawWindow", "()V");
			jCancelDrawWindow.setup(jEnv, idleHelperCls, "cancelDrawWindow", "()V");
			{
				JNINativeMethod method[] =
				{
						{"drawWindow", "()Z", (void*)&Base::drawWindowIdleJNI},
				};
				jEnv->RegisterNatives(idleHelperCls, method, sizeofArray(method));
			}
		}
		else
		{
			int ret = ALooper_addFd(aLooper, drawWinEventFd, ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT, drawWinEventFdHandler, nullptr);
			assert(ret == 1);
		}
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

static bool handleInputEvent(AInputQueue *inputQueue, AInputEvent* event)
{
	#ifdef CONFIG_INPUT
		//logMsg("input event start");
		if(Input::sendInputToIME && AInputQueue_preDispatchEvent(inputQueue, event))
		{
			//logMsg("input event used by pre-dispatch");
			return 1;
		}
		auto handled = Input::onInputEvent(event);
		AInputQueue_finishEvent(inputQueue, event, handled);
		//logMsg("input event end: %s", handled ? "handled" : "not handled");
	#else
		AInputQueue_finishEvent(inputQueue, event, 0);
	#endif
	return 0;
}

// Use on Android 4.1+ to fix a possible ANR where the OS
// claims we haven't processed all input events even though we have.
// This only seems to happen under heavy input event load, like
// when using multiple joysticks. Everything seems to work
// properly if we keep calling AInputQueue_getEvent until
// it returns an error instead of using AInputQueue_hasEvents
// and no warnings are printed to logcat unlike earlier
// Android versions
static void processInputWithGetEvent(AInputQueue *inputQueue)
{
	int events = 0;
	AInputEvent* event = nullptr;
	while(AInputQueue_getEvent(inputQueue, &event) >= 0)
	{
		handleInputEvent(inputQueue, event);
		events++;
	}
	if(events > 1)
	{
		//logMsg("processed %d input events", events);
	}
}

static void processInputWithHasEvents(AInputQueue *inputQueue)
{
	int events = 0;
	int32_t hasEventsRet;
	// Note: never call AInputQueue_hasEvents on first iteration since it may return 0 even if
	// events are present if they were pre-dispatched, leading to an endless stream of callbacks
	do
	{
		AInputEvent* event = nullptr;
		if(AInputQueue_getEvent(inputQueue, &event) < 0)
		{
			logWarn("error getting input event from queue");
			break;
		}
		handleInputEvent(inputQueue, event);
		events++;
	} while((hasEventsRet = AInputQueue_hasEvents(inputQueue)) == 1);
	if(events > 1)
	{
		//logMsg("processed %d input events", events);
	}
	if(hasEventsRet < 0)
	{
		logWarn("error %d in AInputQueue_hasEvents", hasEventsRet);
	}
}

JNIEnv* eEnv() { assert(eJEnv); return eJEnv; }

jobject jniThreadNewGlobalRef(JNIEnv* jEnv, jobject obj)
{
	return jEnv->NewGlobalRef(obj);
}

void jniThreadDeleteGlobalRef(JNIEnv* jEnv, jobject obj)
{
	jEnv->DeleteGlobalRef(obj);
}

void addPollEvent(int fd, PollEventDelegate &handler, uint events)
{
	addPollEvent(aLooper, fd, handler, events);
}

void modPollEvent(int fd, PollEventDelegate &handler, uint events)
{
	addPollEvent(aLooper, fd, handler, events);
}

void removePollEvent(int fd)
{
	removePollEvent(aLooper, fd);
}

void sendMessageToMain(int type, int shortArg, int intArg, int intArg2)
{
	assert(writeMsgPipe != -1);
	uint16 shortArg16 = shortArg;
	int msg[3] = { (shortArg16 << 16) | type, intArg, intArg2 };
	logMsg("sending msg type %d with args %d %d %d", msg[0] & 0xFFFF, msg[0] >> 16, msg[1], msg[2]);
	if(::write(writeMsgPipe, &msg, sizeof(msg)) != sizeof(msg))
	{
		logErr("unable to write message to pipe: %s", strerror(errno));
	}
}

void sendMessageToMain(ThreadPThread &, int type, int shortArg, int intArg, int intArg2)
{
	sendMessageToMain(type, shortArg, intArg, intArg2);
}

#ifdef CONFIG_BLUETOOTH_ANDROID
static const ushort MSG_BT_DATA = 150;

void sendBTSocketData(BluetoothSocket &socket, int len, jbyte *data)
{
	int msg[3] = { MSG_BT_DATA, (int)&socket, len };
	if(::write(writeMsgPipe, &msg, sizeof(msg)) != sizeof(msg))
	{
		logErr("unable to write message header to pipe: %s", strerror(errno));
	}
	if(::write(writeMsgPipe, data, len) != len)
	{
		logErr("unable to write bt data to pipe: %s", strerror(errno));
	}
}
#endif

void setIdleDisplayPowerSave(bool on)
{
	jint keepOn = !on;
	logMsg("keep screen on: %d", keepOn);
	jSetKeepScreenOn(eEnv(), jBaseActivity, keepOn);
}

void setOSNavigationStyle(uint flags)
{
	// Flags mapped directly
	// OS_NAV_STYLE_DIM -> SYSTEM_UI_FLAG_LOW_PROFILE (1)
	// OS_NAV_STYLE_HIDDEN -> SYSTEM_UI_FLAG_HIDE_NAVIGATION (2)
	// 0 -> SYSTEM_UI_FLAG_VISIBLE
	logMsg("setting UI visibility: 0x%X", flags);
	jSetUIVisibility(eEnv(), jBaseActivity, flags);
}

void setProcessPriority(int nice)
{
	assert(nice > -20);
	logMsg("setting process nice level: %d", nice);
	setpriority(PRIO_PROCESS, 0, nice);
}

void setStatusBarHidden(uint hidden)
{
	hidden = hidden ? 1 : 0;
	if(hidden != statusBarIsHidden)
	{
		statusBarIsHidden = hidden;
		logMsg("setting app window fullscreen: %d", (int)statusBarIsHidden);
		jSetFullscreen(eEnv(), jBaseActivity, hidden);
	}
}

bool supportsFrameTime()
{
	return hasChoreographer;
}

static bool drawWindow(int64 frameTimeNanos)
{
	if(!gfxUpdate)
	{
		bug_exit("didn't request graphics update");
	}
	//logMsg("called drawWindow");

	runEngine(frameTimeNanos);
	if(unlikely(didDrawWindowCallback != nullptr))
		didDrawWindowCallback();
	if(gfxUpdate)
	{
		winDrawPosted = 1;
		return 1;
	}
	else
	{
		winDrawPosted = 0;
		return 0;
	}
}

static void postDrawWindow()
{
	if(!winDrawPosted && nWin)
	{
		//logMsg("posted draw");
		if(jPostDrawWindow.m)
			jPostDrawWindow(eEnv(), drawWindowHelper);
		else
		{
			uint64_t post = 1;
			auto ret = write(drawWinEventFd, &post, sizeof(post));
			assert(ret == sizeof(post));
		}
		winDrawPosted = 1;
	}
}

void postDrawWindowIfNeeded()
{
	if(gfxUpdate)
		postDrawWindow();
}

static void cancelDrawWindow()
{
	if(winDrawPosted)
	{
		logMsg("canceled draw");
		if(jCancelDrawWindow.m)
			jCancelDrawWindow(eEnv(), drawWindowHelper);
		else
		{
			uint64_t post;
			read(drawWinEventFd, &post, sizeof(post));
			drawWinEventIdle = 1; // force handler to idle since it could already be signaled by epoll
		}
		winDrawPosted = 0;
	}
}

void restoreOpenGLContext()
{
	eglWin.restoreContext();
}

static int processInputCallback(int fd, int events, void* data)
{
	processInput((AInputQueue*)data);
	postDrawWindowIfNeeded();
	return 1;
}

static int drawWinEventFdHandler(int fd, int events, void* data)
{
	// this callback should behave as the "idle-handler" so input-related fds are processed in a timely manner
	if(drawWinEventIdle)
	{
		//logMsg("idled");
		drawWinEventIdle--;
		return 1;
	}
	else
	{
		// "idle" every other call so other fds are processed
		// to avoid a frame of input lag
		drawWinEventIdle = 1;
	}

	if(likely(inputQueue != nullptr) && AInputQueue_hasEvents(inputQueue) == 1)
	{
		// some devices may delay reporting input events (stock rom on R800i for example),
		// check for any before rendering frame to avoid extra latency
		processInput(inputQueue);
	}

	if(!drawWindow(0))
	{
		uint64_t post;
		auto ret = read(drawWinEventFd, &post, sizeof(post));
		assert(ret == sizeof(post));
	}
	return 1;
}

static int msgPipeFdHandler(int fd, int events, void* data)
{
	while(fd_bytesReadable(fd))
	{
		uint32 cmd;
		if(read(fd, &cmd, sizeof(cmd)) != sizeof(cmd))
		{
			logErr("error reading command in message pipe");
			return 1;
		}

		uint cmdType = cmd & 0xFFFF;
		logMsg("got thread message %d", cmdType);
		switch(cmdType)
		{
			#ifdef CONFIG_BLUETOOTH_ANDROID
			bcase MSG_BT_DATA:
			{
				BluetoothSocket *s;
				read(fd, &s, sizeof(s));
				int size;
				read(fd, &size, sizeof(size));
				uchar buff[48];
				read(fd, buff, size);
				s->onData()(buff, size);
			}
			bcase MSG_BT_SOCKET_STATUS_DELEGATE:
			{
				logMsg("got bluetooth socket status delegate message");
				uint32 arg[2];
				read(fd, arg, sizeof(arg));
				auto &s = *((AndroidBluetoothSocket*)arg[1]);
				s.onStatusDelegateMessage(arg[0]);
			}
			#endif
			bdefault:
			if(cmdType >= MSG_IMAGINE_START)
			{
				uint32 arg[2];
				read(fd, arg, sizeof(arg));
				logMsg("got msg type %d with args %d %d %d", cmdType, cmd >> 16, arg[0], arg[1]);
				Base::processAppMsg(cmdType, cmd >> 16, arg[0], arg[1]);
			}
			else
				logWarn("got unknown cmd %d", cmd);
			break;
		}
	}
	postDrawWindowIfNeeded();
	return 1;
}

static jboolean JNICALL drawWindowJNI(JNIEnv* env, jobject thiz, jlong frameTimeNanos)
{
	//logMsg("frame time %lld, diff %lld", (long long)frameTimeNanos, (long long)(frameTimeNanos - lastFrameTime));
	prevFrameTimeNanos = frameTimeNanos;
	return drawWindow(frameTimeNanos);
}

static jboolean JNICALL drawWindowIdleJNI(JNIEnv* env, jobject thiz)
{
	return drawWindow(0);
}

static void onDestroy(ANativeActivity* activity)
{
	::exit(0);
}

static void onStart(ANativeActivity* activity)
{
}

static void onResume(ANativeActivity* activity)
{
	appResumed();
	postDrawWindowIfNeeded();
}

static void* onSaveInstanceState(ANativeActivity* activity, size_t* outLen)
{
	return nullptr;
}

static void onPause(ANativeActivity* activity)
{
	appPaused();
	gfxUpdate = 0;
	cancelDrawWindow();
}

static void onStop(ANativeActivity* activity)
{
}

static void onConfigurationChanged(ANativeActivity* activity)
{
	AConfiguration_fromAssetManager(aConfig, activity->assetManager);
	configChange(activity->env, aConfig, nWin);
	postDrawWindowIfNeeded();
}

static void onLowMemory(ANativeActivity* activity)
{
}

static void onWindowFocusChanged(ANativeActivity* activity, int focused)
{
	appFocus(focused, nWin);
	postDrawWindowIfNeeded();
}

static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window)
{
	nWin = window;
	initWindow(activity, window);
	gfxUpdate = 1;
	postDrawWindow();
}

static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window)
{
	eglWin.destroySurface();
	nWin = nullptr;
	gfxUpdate = 0;
	cancelDrawWindow();
	if(Base::androidSDK() < 11)
	{
		// In testing with CM7 on a Droid, the surface is re-created in RGBA8888 upon
		// resuming the app and ANativeWindow_setBuffersGeometry() has no effect.
		// Explicitly setting the format here seems to fix the problem. Android bug?
		logMsg("explicitly setting destroyed window format to %d", eglWin.currentWindowFormat());
		ANativeActivity_setWindowFormat(activity, eglWin.currentWindowFormat());
	}
}

static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue)
{
	inputQueue = queue;
	if(engineIsInit)
	{
		logMsg("input queue created, attaching to looper");
		AInputQueue_attachLooper(queue, aLooper, ALOOPER_POLL_CALLBACK, processInputCallback, queue);
	}
	else
	{
		logMsg("input queue created, waiting for initial window creation to attach");
	}
}

static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue)
{
	logMsg("input queue destroyed");
	inputQueue = nullptr;
	AInputQueue_detachLooper(queue);
}

static void onNativeWindowResized(ANativeActivity* activity, ANativeWindow* window)
{
	windowResized(window);
	postDrawWindowIfNeeded();
}

static void onNativeWindowRedrawNeeded(ANativeActivity* activity, ANativeWindow* window)
{
	windowNeedsRedraw(window);
}

static void onContentRectChanged(ANativeActivity* activity, const ARect* rect)
{
	contentRectChanged(*rect, nWin);
}

}

CLINK void LVISIBLE ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
	logMsg("called ANativeActivity_onCreate, thread ID %d", gettid());
	using namespace Base;
	setSDK(activity->sdkVersion);
	if(Base::androidSDK() >= 16)
		processInput = Base::processInputWithGetEvent;
	aLooper = ALooper_forThread();
	assert(aLooper);
	jVM = activity->vm;
	jBaseActivity = activity->clazz;
	eJEnv = activity->env;
	activityInit(activity);
	Base::dlLoadFuncs();

	#ifdef CONFIG_INPUT
	if(Base::androidSDK() >= 12)
	{
		logMsg("setting up inotify");
		int inputDevNotifyFd = inotify_init();
		if(inputDevNotifyFd >= 0)
		{
			auto watch = inotify_add_watch(inputDevNotifyFd, "/dev/input", IN_CREATE | IN_DELETE );
			int ret = ALooper_addFd(aLooper, inputDevNotifyFd, ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT, inputDevNotifyFdHandler, nullptr);
			assert(ret == 1);
		}
		else
		{
			logErr("couldn't create inotify instance");
		}
	}

	doOrExit(Input::init());
	#endif

	aConfig = AConfiguration_new();
	AConfiguration_fromAssetManager(aConfig, activity->assetManager);
	initConfig(aConfig);

	int msgPipe[2];
	{
		int ret = pipe(msgPipe);
		assert(ret == 0);
		ret = ALooper_addFd(aLooper, msgPipe[0], ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT, msgPipeFdHandler, nullptr);
		assert(ret == 1);
	}
	writeMsgPipe = msgPipe[1];

	activity->callbacks->onDestroy = onDestroy;
	//activity->callbacks->onStart = onStart;
	//activity->callbacks->onResume = onResume;
	//activity->callbacks->onSaveInstanceState = onSaveInstanceState;
	//activity->callbacks->onPause = onPause;
	activity->callbacks->onStop = onStop;
	activity->callbacks->onConfigurationChanged = onConfigurationChanged;
	//activity->callbacks->onLowMemory = onLowMemory;
	activity->callbacks->onWindowFocusChanged = onWindowFocusChanged;
	activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
	activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
	activity->callbacks->onNativeWindowResized = onNativeWindowResized;
	activity->callbacks->onNativeWindowRedrawNeeded = onNativeWindowRedrawNeeded;
	activity->callbacks->onInputQueueCreated = onInputQueueCreated;
	activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
	activity->callbacks->onContentRectChanged = onContentRectChanged;
}
