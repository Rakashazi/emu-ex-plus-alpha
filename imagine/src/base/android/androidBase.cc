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

#define LOGTAG "Base"
#include <cstdlib>
#include <errno.h>

#include <logger/interface.h>
#include <engine-globals.h>
#include <base/android/sdk.hh>
#include <base/Base.hh>
#include <base/Timer.hh>
#include <base/common/windowPrivate.hh>
#include <base/common/funcs.h>
#include <base/android/ASurface.hh>
#include <gfx/Gfx.hh>
#include <input/android/private.hh>
#include <android/window.h>
#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>
#include <dlfcn.h>
#include <sys/eventfd.h>
#include <fs/sys.hh>
#include <util/fd-utils.h>
#include <util/bits.h>
#ifdef CONFIG_BLUETOOTH
#include <bluetooth/BluetoothInputDevScanner.hh>
#endif
#include "private.hh"
#include "EGLContextHelper.hh"

bool glSyncHackEnabled = 0, glSyncHackBlacklisted = 0;
bool glPointerStateHack = 0, glBrokenNpot = 0;

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

JavaVM* jVM = nullptr;
static JNIEnv* eJEnv = nullptr;
static const char *buildDevice = nullptr;

// input
AInputQueue *inputQueue = nullptr;
static int aHardKeyboardState = 0, aKeyboardType = ACONFIGURATION_KEYBOARD_NOKEYS, aHasHardKeyboard = 0;
static void processInputWithGetEvent(AInputQueue *inputQueue);
static void processInputWithHasEvents(AInputQueue *inputQueue);
static void (*processInput)(AInputQueue *inputQueue) = Base::processInputWithHasEvents;

// activity
jclass jBaseActivityCls = nullptr;
jobject jBaseActivity = nullptr;
static ANativeActivity *baseActivity = nullptr;
uint appState = APP_PAUSED;
bool aHasFocus = true;
bool resumeAppOnWindowInit = false;
static AConfiguration *aConfig = nullptr;
static JavaInstMethod<void> jSetUIVisibility;
//static JavaInstMethod<void> jFinish;
static JavaInstMethod<jobject> jIntentDataPath;
static JavaInstMethod<jobject> jNewFontRenderer;
static JavaInstMethod<void> jAddViewShortcut;
JavaInstMethod<void> jSetRequestedOrientation;
static JavaInstMethod<void> jAddNotification, jRemoveNotification;
static jobject vibrator = nullptr;
static JavaInstMethod<void> jVibrate;
static JavaInstMethod<jboolean> jPackageIsInstalled;
const char *appPath = nullptr;
static const char *filesDir = nullptr, *eStoreDir = nullptr;
static uint aSDK = aMinSDK;
static bool osAnimatesRotation = false;
int osOrientation = -1;
static bool trackballNav = false;
static bool hasPermanentMenuKey = true;
static bool keepScreenOn = false;
static Timer userActivityCallback;
static uint uiVisibilityFlags = SYS_UI_STYLE_NO_FLAGS;
void onResume(ANativeActivity* activity);
void onPause(ANativeActivity* activity);
TimeSys orientationEventTime;

// window
jobject frameHelper = nullptr;
static bool hasChoreographer = 0;
static int64 prevFrameTimeNanos = 0;
EGLContextHelper eglCtx;
EGLDisplay display = EGL_NO_DISPLAY;
JavaInstMethod<void> jSetWinFormat, jSetWinFlags;
JavaInstMethod<int> jWinFormat, jWinFlags;
JavaInstMethod<void> jPostFrame, jUnpostFrame;
int onFrameEventFd = -1;
uint onFrameEventIdle = 0;
bool onFrame(int64 frameTimeNanos, bool forceDraw);
void windowNeedsRedraw(ANativeActivity* activity, ANativeWindow *win);
void windowResized(ANativeActivity* activity, ANativeWindow *win);
void contentRectChanged(ANativeActivity* activity, const ARect &rect, ANativeWindow *win);
void finishWindowInit(ANativeActivity* activity, ANativeWindow *win, bool hasFocus);
void windowDestroyed(ANativeActivity* activity, Window &win);
void addOnFrameWindowSizeChecks(uint extraRedraws);
void setupEGLConfig();

// display
static JavaInstMethod<jint> jGetRotation;
static JavaInstMethod<jfloat> jGetRefreshRate;
static jobject jDpy = nullptr;
static uint refreshRate_ = 0;
float androidXDPI = 0, androidYDPI = 0; // DPI reported by OS
float aDensityDPI = 0;
uint androidUIInUse = 0;

static Base::Screen::OnFrameDelegate restoreGLContextFromAndroidUI
{
	[](Screen &screen, FrameTimeBase)
	{
		// an Android UI element like EditText may make its
		// own context current so we must check and restore ours
		restoreOpenGLContext();
		if(androidUIInUse)
		{
			// do callback each frame until UI use stops
			screen.addOnFrameDelegate(restoreGLContextFromAndroidUI);
		}
		else
		{
			logMsg("stopping per-frame context check");
		}
	}
};

void unrefUIGL()
{
	assert(androidUIInUse);
	androidUIInUse--;
	if(!androidUIInUse)
	{
		//logMsg("no more Android UI elements in use");
	}
}

void refUIGL()
{
	androidUIInUse++;
	auto &screen = mainScreen();
	if(!screen.containsOnFrameDelegate(restoreGLContextFromAndroidUI))
	{
		screen.addOnFrameDelegate(restoreGLContextFromAndroidUI);
	}
}

uint appActivityState() { return appState; }

void exit(int returnVal)
{
	// TODO: return exit value as activity result
	logMsg("exiting process");
	appState = APP_EXITING;
	onExit(false);
	auto env = eEnv();
	jRemoveNotification(env, jBaseActivity);
	::exit(returnVal);
	//jFinish(env, jBaseActivity);
}
void abort() { ::abort(); }

//void openURL(const char *url) { };

const char *documentsPath() { return filesDir; }
const char *storagePath() { return eStoreDir; }

AAssetManager *activityAAssetManager()
{
	assert(baseActivity);
	assert(baseActivity->assetManager);
	return baseActivity->assetManager;
}

bool hasHardwareNavButtons()
{
	return hasPermanentMenuKey;
}

#if !defined CONFIG_MACHINE_OUYA
static void setupVibration(JNIEnv* jEnv)
{
	if(!jVibrate)
	{
		//logMsg("setting up Vibrator class");
		auto vibratorCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("android/os/Vibrator"));
		jVibrate.setup(jEnv, vibratorCls, "vibrate", "(J)V");
	}
}

bool hasVibrator()
{
	return vibrator;
}

void vibrate(uint ms)
{
	if(unlikely(!vibrator))
		return;
	if(unlikely(!jVibrate))
	{
		setupVibration(eEnv());
	}

	//logDMsg("vibrating for %u ms", ms);
	jVibrate(eEnv(), vibrator, (jlong)ms);
}
#endif

void addNotification(const char *onShow, const char *title, const char *message)
{
	logMsg("adding notificaion icon");
	jAddNotification(eEnv(), jBaseActivity, eEnv()->NewStringUTF(onShow), eEnv()->NewStringUTF(title), eEnv()->NewStringUTF(message));
}

void addLauncherIcon(const char *name, const char *path)
{
	logMsg("adding launcher icon: %s, for path: %s", name, path);
	jAddViewShortcut(eEnv(), jBaseActivity, eEnv()->NewStringUTF(name), eEnv()->NewStringUTF(path));
}

void setSDK(uint sdk)
{
	aSDK = sdk;
}

uint androidSDK()
{
	return std::max(aMinSDK, aSDK);
}

static void setDeviceType(const char *dev)
{
	if(Config::MACHINE_IS_GENERIC_ARMV7)
	{
		// TODO: confirm this isn't needed on newest Evo 3D/Shooter firmware
		if(androidSDK() < 11 && (/*strstr(dev, "shooter") ||*/ string_equal(dev, "inc")))
		{
			// HTC Droid Incredible hack
			logMsg("device needs glFinish() hack");
			glSyncHackBlacklisted = 1;
		}
		// TODO: re-test for broken NPOT using OpenGL ES 2.0 renderer
		/*else if(androidSDK() < 11 && (string_equal(dev, "vision") || string_equal(dev, "ace")))
		{
			// T-Mobile G2 (HTC Desire Z), HTC Desire HD
			logMsg("device has broken npot support");
			glBrokenNpot = 1;
		}*/
	}
	// TODO: is this still needed?
	/*if(androidSDK() < 11 && string_equal(dev, "GT-B5510"))
	{
		logMsg("device needs gl*Pointer() hack");
		glPointerStateHack = 1;
	}*/
}

// NAVHIDDEN_* mirrors KEYSHIDDEN_*

static const char *hardKeyboardNavStateToStr(int state)
{
	switch(state)
	{
		case ACONFIGURATION_KEYSHIDDEN_ANY: return "undefined";
		case ACONFIGURATION_KEYSHIDDEN_NO: return "no";
		case ACONFIGURATION_KEYSHIDDEN_YES: return "yes";
		case ACONFIGURATION_KEYSHIDDEN_SOFT:  return "soft";
		default: return "unknown";
	}
}

bool hasHardKeyboard() { return aHasHardKeyboard; }
int hardKeyboardState() { return aHardKeyboardState; }

static void setHardKeyboardState(int hardKeyboardState)
{
	if(aHardKeyboardState != hardKeyboardState)
	{
		aHardKeyboardState = hardKeyboardState;
		logMsg("hard keyboard hidden: %s", hardKeyboardNavStateToStr(aHardKeyboardState));
		#ifdef CONFIG_INPUT
		Input::setBuiltInKeyboardState(aHardKeyboardState == ACONFIGURATION_KEYSHIDDEN_NO);
		#endif
	}
}

static void setKeyboardType(int keyboardType)
{
	if(aKeyboardType != keyboardType)
	{
		logMsg("keyboard changed: %d", keyboardType);
		aKeyboardType = keyboardType;
	}
}

int keyboardType()
{
	return aKeyboardType;
}

static bool setOrientationOS(int o)
{
	using namespace Gfx;
	static const Angle orientationDiffTable[4][4] =
	{
			{ 0, angleFromDegree(90), angleFromDegree(-180), angleFromDegree(-90) },
			{ angleFromDegree(-90), 0, angleFromDegree(90), angleFromDegree(-180) },
			{ angleFromDegree(-180), angleFromDegree(-90), 0, angleFromDegree(90) },
			{ angleFromDegree(90), angleFromDegree(-180), angleFromDegree(-90), 0 },
	};

	logMsg("OS orientation change");
	orientationEventTime = orientationEventTime.now();
	assert(osOrientation != -1);
	if(osOrientation != o)
	{
		if(!osAnimatesRotation)
		{
			auto rotAngle = orientationDiffTable[osOrientation][o];
			logMsg("animating from %d degrees", (int)angleToDegree(rotAngle));
			//projAngleM.set(IG::toRadians(rotAngle), 0., INTERPOLATOR_TYPE_EASEOUTQUAD, 10);
			Gfx::animateProjectionMatrixRotation(rotAngle, 0.);
		}
		//logMsg("new value %d", o);
		osOrientation = o;
		return 1;
	}
	return 0;

}

const char *androidBuildDevice()
{
	assert(buildDevice);
	return buildDevice;
}

uint Screen::refreshRate()
{
	if(!refreshRate_)
	{
		assert(jDpy);
		refreshRate_ = jGetRefreshRate(eEnv(), jDpy);
		logMsg("refresh rate: %d", refreshRate_);
	}
	return refreshRate_;
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
	bool sigMatchesAPK = true;
	#ifdef ANDROID_APK_SIGNATURE_HASH
	JavaInstMethod<jint> jSigHash;
	auto jEnv = eEnv();
	jSigHash.setup(jEnv, jBaseActivityCls, "sigHash", "()I");
	sigMatchesAPK = jSigHash(jEnv, jBaseActivity) == ANDROID_APK_SIGNATURE_HASH;
	#endif
	return sigMatchesAPK;
}

bool packageIsInstalled(const char *name)
{
	auto jEnv = eEnv();
	if(!jPackageIsInstalled)
		jPackageIsInstalled.setup(jEnv, jBaseActivityCls, "packageIsInstalled", "(Ljava/lang/String;)Z");
	return jPackageIsInstalled(jEnv, jBaseActivity, jEnv->NewStringUTF(name));
}

EGLDisplay getAndroidEGLDisplay()
{
	assert(display != EGL_NO_DISPLAY);
	return display;
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
	trackballNav = AConfiguration_getNavigation(config) == ACONFIGURATION_NAVIGATION_TRACKBALL;
	if(trackballNav)
		logMsg("detected trackball");

	aHardKeyboardState = Input::hasXperiaPlayGamepad() ? navigationState : hardKeyboardState;
	logMsg("keyboard/nav hidden: %s", hardKeyboardNavStateToStr(aHardKeyboardState));

	aKeyboardType = keyboard;
	if(aKeyboardType != ACONFIGURATION_KEYBOARD_NOKEYS)
		logMsg("keyboard type: %d", aKeyboardType);
}

static void appFocus(bool hasFocus, ANativeWindow* window)
{
	aHasFocus = hasFocus;
	logMsg("focus change: %d", (int)hasFocus);
	if(hasFocus && window)
	{
		logMsg("app in focus, window size %d,%d", ANativeWindow_getWidth(window), ANativeWindow_getHeight(window));
		mainWindow().postDraw();
		addOnFrameWindowSizeChecks(0);
	}
	if(eglCtx.isInit())
		onFocusChange(mainWindow(), hasFocus);
}

static void configChange(JNIEnv* jEnv, AConfiguration* config, ANativeWindow* window)
{
	auto hardKeyboardState = AConfiguration_getKeysHidden(config);
	auto navState = AConfiguration_getNavHidden(config);
	auto orientation = jGetRotation(jEnv, jDpy);
	auto keyboard = AConfiguration_getKeyboard(config);
	//trackballNav = AConfiguration_getNavigation(config) == ACONFIGURATION_NAVIGATION_TRACKBALL;
	logMsg("config change, keyboard: %s, navigation: %s", hardKeyboardNavStateToStr(hardKeyboardState), hardKeyboardNavStateToStr(navState));
	setHardKeyboardState(Input::hasXperiaPlayGamepad() ? navState : hardKeyboardState);

	if(setOrientationOS(orientation))
	{
		//logMsg("changed OS orientation");
	}
}

bool hasTrackball()
{
	return trackballNav;
}

static void appPaused()
{
	if(appIsRunning())
		appState = APP_PAUSED;
	logMsg("app %s", appState == APP_PAUSED ? "paused" : "exiting");
	onExit(appState == APP_PAUSED);
	#ifdef CONFIG_AUDIO
	Audio::updateFocusOnPause();
	#endif
	#ifdef CONFIG_INPUT_ANDROID_MOGA
	Input::onPauseMOGA(eEnv());
	#endif
	Input::deinitKeyRepeatTimer();
	mainScreen().unpostFrame();
}

void handleIntent(ANativeActivity* activity)
{
	// check for view intents
	auto jEnv = activity->env;
	jstring intentDataPathJStr = (jstring)jIntentDataPath(jEnv, activity->clazz);
	if(intentDataPathJStr)
	{
		const char *intentDataPathStr = jEnv->GetStringUTFChars(intentDataPathJStr, nullptr);
		logMsg("got intent with path: %s", intentDataPathStr);
		onInterProcessMessage(intentDataPathStr);
		jEnv->ReleaseStringUTFChars(intentDataPathJStr, intentDataPathStr);
		jEnv->DeleteLocalRef(intentDataPathJStr);
	}
}

void doOnResume(ANativeActivity* activity)
{
	#ifdef CONFIG_INPUT_ANDROID_MOGA
	Input::onResumeMOGA(eEnv(), true);
	#endif
	onResume(aHasFocus);
	handleIntent(activity);
}

static void appResumed(ANativeActivity* activity)
{
	appState = APP_RUNNING;
	#ifdef CONFIG_AUDIO
	Audio::updateFocusOnResume();
	#endif
	if(mainWindow())
	{
		logMsg("app resumed");
		// check if a window needs to draw since the surface
		// may have been created before onResume()
		if(mainWindow().needsDraw())
		{
			mainScreen().postFrame();
		}
		doOnResume(activity);
	}
	else
	{
		logMsg("app resumed without window, delaying onResume handler");
		resumeAppOnWindowInit = 1;
	}
}

static void initEGL()
{
	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(display != EGL_NO_DISPLAY);
	eglInitialize(display, 0, 0);
	logMsg("%s (%s), extensions: %s", eglQueryString(display, EGL_VENDOR), eglQueryString(display, EGL_VERSION), eglQueryString(display, EGL_EXTENSIONS));

	//ANativeWindow_setBuffersGeometry(win, 0, 0, configFormat);
	//printEGLConfs(display);
	//printEGLConfsWithAttr(display, eglAttrWinMaxRGBA);
	//printEGLConfsWithAttr(display, eglAttrWinRGB888);
	//printEGLConfsWithAttr(display, eglAttrWinLowColor);
}

static void activityInit(ANativeActivity* activity) // uses JNIEnv from Activity thread
{
	auto jEnv = activity->env;
	auto inst = activity->clazz;
	using namespace Base;
	//logMsg("doing app creation");

	// BaseActivity members
	{
		jBaseActivityCls = (jclass)jEnv->NewGlobalRef(jEnv->GetObjectClass(inst));
		jSetRequestedOrientation.setup(jEnv, jBaseActivityCls, "setRequestedOrientation", "(I)V");
		jAddNotification.setup(jEnv, jBaseActivityCls, "addNotification", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
		jRemoveNotification.setup(jEnv, jBaseActivityCls, "removeNotification", "()V");
		jIntentDataPath.setup(jEnv, jBaseActivityCls, "intentDataPath", "()Ljava/lang/String;");
		jAddViewShortcut.setup(jEnv, jBaseActivityCls, "addViewShortcut", "(Ljava/lang/String;Ljava/lang/String;)V");
		//jFinish.setup(jEnv, jBaseActivityCls, "finish", "()V");
		#ifdef CONFIG_RESOURCE_FONT_ANDROID
		jNewFontRenderer.setup(jEnv, jBaseActivityCls, "newFontRenderer", "()Lcom/imagine/FontRenderer;");
		#endif

		{
			JNINativeMethod method[] =
			{
				{
					"onContentRectChanged", "(IIII)V",
					(void*)(void JNICALL (*)(JNIEnv* env, jobject thiz, jint x, jint y, jint x2, jint y2))
					([](JNIEnv* env, jobject thiz, jint x, jint y, jint x2, jint y2)
					{
						contentRectChanged(baseActivity, {x, y, x2, y2}, mainWindow().nWin);
					})
				}
			};
			jEnv->RegisterNatives(jBaseActivityCls, method, sizeofArray(method));
		}

		if(Base::androidSDK() < 11) // bug in pre-3.0 Android causes paths in ANativeActivity to be null
		{
			logMsg("ignoring paths from ANativeActivity due to Android 2.3 bug");
			JavaInstMethod<jobject> jFilesDir;
			jFilesDir.setup(jEnv, jBaseActivityCls, "filesDir", "()Ljava/lang/String;");
			filesDir = jEnv->GetStringUTFChars((jstring)jFilesDir(jEnv, inst), nullptr);
		}
		else
		{
			filesDir = activity->internalDataPath;
			//eStoreDir = activity->externalDataPath;
		}

		{
			JavaClassMethod<jobject> extStorageDir;
			extStorageDir.setup(jEnv, jBaseActivityCls, "extStorageDir", "()Ljava/lang/String;");
			eStoreDir = jEnv->GetStringUTFChars((jstring)extStorageDir(jEnv), nullptr);
			assert(filesDir);
			assert(eStoreDir);
			logMsg("internal storage path: %s", filesDir);
			logMsg("external storage path: %s", eStoreDir);
		}

		doOrAbort(logger_init());
		engineInit();
		logMsg("SDK API Level: %d", aSDK);

		{
			JavaInstMethod<jobject> jApkPath;
			jApkPath.setup(jEnv, jBaseActivityCls, "apkPath", "()Ljava/lang/String;");
			appPath = jEnv->GetStringUTFChars((jstring)jApkPath(jEnv, inst), nullptr);
			logMsg("apk @ %s", appPath);
		}

		{
			JavaClassMethod<jobject> jDevName;
			jDevName.setup(jEnv, jBaseActivityCls, "devName", "()Ljava/lang/String;");
			auto devName = (jstring)jDevName(jEnv);
			buildDevice = jEnv->GetStringUTFChars(devName, nullptr);
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
	bool isStraightOrientation = !ASurface::isSidewaysOrientation(orientation);

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
//	if(Config::MACHINE_IS_GENERIC_ARMV7 && Base::androidSDK() >= 16 && (strstr(buildDevice, "spyder") || strstr(buildDevice, "targa")))
//	{
//		logMsg("using scaled DPI as physical DPI due to Droid RAZR/Bionic Android 4.1 bug");
//		metricsXDPI = metricsYDPI = aDensityDPI;
//	}
	// DPI values are un-rotated from DisplayMetrics
	androidXDPI = isStraightOrientation ? metricsXDPI : metricsYDPI;
	androidYDPI = isStraightOrientation ? metricsYDPI : metricsXDPI;

	if(Config::MACHINE_IS_GENERIC_ARMV7 && Base::androidSDK() >= 11)
	{
		// TODO: need to re-test with OpenGL ES 2.0 backend
		/*if(FsSys::fileExists("/system/lib/egl/libEGL_adreno200.so"))
		{
			// Hack for Adreno chips that have display artifacts when using 32-bit color.
			logMsg("device may have broken 32-bit surfaces, defaulting to low color");
			eglCtx.useMaxColorBits = 0;
			eglCtx.has32BppColorBugs = 1;
		}
		else*/
		{
			logMsg("defaulting to highest color mode");
			eglCtx.useMaxColorBits = 1;
		}
	}

	//jSetKeepScreenOn.setup(jEnv, jBaseActivityCls, "setKeepScreenOn", "(Z)V");
	jSetWinFlags.setup(jEnv, jBaseActivityCls, "setWinFlags", "(II)V");
	jWinFlags.setup(jEnv, jBaseActivityCls, "winFlags", "()I");
	if(Base::androidSDK() < 11)
	{
		jSetWinFormat.setup(jEnv, jBaseActivityCls, "setWinFormat", "(I)V");
		jWinFormat.setup(jEnv, jBaseActivityCls, "winFormat", "()I");
	}
	jSetUIVisibility.setup(jEnv, jBaseActivityCls, "setUIVisibility", "(I)V");

	// ui visibility change notifications
	if(Base::androidSDK() >= 11)
	{
		logMsg("setting up UI visibility change notifications");
		JavaInstMethod<jobject> jUIVisibilityChangeHelper;
		jUIVisibilityChangeHelper.setup(jEnv, Base::jBaseActivityCls, "uiVisibilityChangeHelper", "()Lcom/imagine/SystemUiVisibilityChangeHelper;");
		auto uiVisibilityChangeHelper = jUIVisibilityChangeHelper(jEnv, Base::jBaseActivity);
		assert(uiVisibilityChangeHelper);
		auto uiVisibilityChangeHelperCls = jEnv->GetObjectClass(uiVisibilityChangeHelper);
		JNINativeMethod method[] =
		{
			{
				"visibilityChange", "(I)V",
				(void*)(void JNICALL (*)(JNIEnv* env, jobject thiz, jint visibility))
				([](JNIEnv* env, jobject thiz, jint visibility)
				{
					int possibleChangeFlags = uiVisibilityFlags & 0x7;
					if(possibleChangeFlags != visibility)
					{
						logMsg("system UI visibility changed from %d to %d, reapplying app's style", possibleChangeFlags, visibility);
						int flags = uiVisibilityFlags;
						if((Base::androidSDK() == 16 || Base::androidSDK() == 17)
							&& (flags & SYS_UI_STYLE_HIDE_NAV) && !(visibility & SYS_UI_STYLE_HIDE_NAV))
						{
							// if we try to re-apply the hide navigation flag in Android 4.1 & 4.2
							// and the cause of the UI visibility change is a screen touch,
							// it won't have an effect and we won't set SYSTEM_UI_FLAG_LAYOUT_STABLE,
							// causing wrong content insets
							logMsg("ignoring hide navigation bit");
							unsetBits(flags, SYS_UI_STYLE_HIDE_NAV);
						}
						jSetUIVisibility(env, jBaseActivity, flags);
					}
				})
			}
		};
		jEnv->RegisterNatives(uiVisibilityChangeHelperCls, method, sizeofArray(method));
	}

	initEGL();
	doOrAbort(onInit(0, nullptr));
	if(!mainWin)
	{
		bug_exit("didn't create a window");
	}
	setupEGLConfig();
	eglCtx.init(display);

	// select display update method
	if(Base::androidSDK() >= 16) // Choreographer
	{
		//logMsg("using Choreographer for display updates");
		hasChoreographer = true;
		JavaInstMethod<jobject> jNewChoreographerHelper;
		jNewChoreographerHelper.setup(jEnv, jBaseActivityCls, "newChoreographerHelper", "()Lcom/imagine/ChoreographerHelper;");
		frameHelper = jNewChoreographerHelper(jEnv, inst);
		assert(frameHelper);
		frameHelper = jEnv->NewGlobalRef(frameHelper);
		auto choreographerHelperCls = jEnv->GetObjectClass(frameHelper);
		jPostFrame.setup(jEnv, choreographerHelperCls, "postFrame", "()V");
		jUnpostFrame.setup(jEnv, choreographerHelperCls, "unpostFrame", "()V");
		JNINativeMethod method[] =
		{
			{
				"onFrame", "(J)Z",
				(void*)(jboolean JNICALL(*)(JNIEnv* env, jobject thiz, jlong frameTimeNanos))
				([](JNIEnv* env, jobject thiz, jlong frameTimeNanos)
				{
					//logMsg("frame time %lld, diff %lld", (long long)frameTimeNanos, (long long)(frameTimeNanos - lastFrameTime));
					assert(mainScreen().frameIsPosted());
					prevFrameTimeNanos = frameTimeNanos;
					return (jboolean)onFrame(frameTimeNanos, false);
				})
			}
		};
		jEnv->RegisterNatives(choreographerHelperCls, method, sizeofArray(method));
	}
	else
	{
		onFrameEventFd = eventfd(0, 0);
		if(unlikely(onFrameEventFd == -1)) // MessageQueue.IdleHandler
		{
			logWarn("error creating eventfd: %d (%s), falling back to idle handler", errno, strerror(errno));
			JavaInstMethod<jobject> jNewIdleHelper;
			jNewIdleHelper.setup(jEnv, jBaseActivityCls, "newIdleHelper", "()Lcom/imagine/BaseActivity$IdleHelper;");
			frameHelper = jNewIdleHelper(jEnv, inst);
			assert(frameHelper);
			frameHelper = jEnv->NewGlobalRef(frameHelper);
			auto idleHelperCls = jEnv->GetObjectClass(frameHelper);
			jPostFrame.setup(jEnv, idleHelperCls, "postFrame", "()V");
			jUnpostFrame.setup(jEnv, idleHelperCls, "unpostFrame", "()V");
			JNINativeMethod method[]
			{
				{
					"onFrame", "()Z",
					(void*)(jboolean JNICALL(*)(JNIEnv* env, jobject thiz))
					([](JNIEnv* env, jobject thiz)
					{
						assert(mainScreen().frameIsPosted());
						return (jboolean)onFrame(0, true); // force window draw so buffers swap to wait until next frame
					})
				}
			};
			jEnv->RegisterNatives(idleHelperCls, method, sizeofArray(method));
		}
		else // eventfd
		{
			// this callback should behave as the "idle-handler" so input-related fds are processed in a timely manner
			int ret = ALooper_addFd(activityLooper(), onFrameEventFd, ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
				[](int fd, int, void*)
				{
					if(onFrameEventIdle)
					{
						//logMsg("idled");
						onFrameEventIdle--;
						return 1;
					}
					else
					{
						// "idle" every other call so other fds are processed
						// to avoid a frame of input lag
						onFrameEventIdle = 1;
					}
					assert(mainScreen().frameIsPosted());
					if(likely(inputQueue) && AInputQueue_hasEvents(inputQueue) == 1)
					{
						// some devices may delay reporting input events (stock rom on R800i for example),
						// check for any before rendering frame to avoid extra latency
						processInput(inputQueue);
					}

					if(!onFrame(0, true)) // force window draw so buffers swap to wait until next frame
					{
						uint64_t post;
						auto ret = read(fd, &post, sizeof(post));
						assert(ret == sizeof(post));
					}
					return 1;
				}, nullptr);
			assert(ret == 1);
		}
	}
}

static void dlLoadFuncs()
{
	#if CONFIG_ENV_ANDROID_MINSDK >= 12
	 // no functions from dlopen needed if targeting at least Android 3.1 (SDK 12)
	return;
	#endif
	 // no functions from dlopen needed if running less than Android 3.1 (SDK 12)
	if(Base::androidSDK() < 12)
	{
		return;
	}

	void *libandroid = dlopen("/system/lib/libandroid.so", RTLD_LOCAL | RTLD_LAZY);
	if(!libandroid)
	{
		logWarn("unable to dlopen libandroid.so");
		return;
	}

	#ifdef CONFIG_INPUT
	Input::dlLoadAndroidFuncs(libandroid);
	#endif
}

static bool handleInputEvent(AInputQueue *inputQueue, AInputEvent* event)
{
	#ifdef CONFIG_INPUT
	//logMsg("input event start");
	if(unlikely(!mainWindow().nWin))
	{
		logMsg("ignoring input with uninitialized window");
		AInputQueue_finishEvent(inputQueue, event, 0);
		return 1;
	}
	if(unlikely(Input::sendInputToIME && AInputQueue_preDispatchEvent(inputQueue, event)))
	{
		//logMsg("input event used by pre-dispatch");
		return 1;
	}
	auto handled = Input::onInputEvent(event, mainWindow());
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

/*void sendMessageToMain(int type, int shortArg, int intArg, int intArg2)
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
}*/

/*#ifdef CONFIG_BLUETOOTH_ANDROID
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
#endif*/

void setIdleDisplayPowerSave(bool on)
{
	auto env = eEnv();
	jint keepOn = !on;
	//jSetKeepScreenOn(eEnv(), jBaseActivity, keepOn);
	auto keepsScreenOn = userActivityCallback ? false : (bool)(jWinFlags(env, jBaseActivity) & AWINDOW_FLAG_KEEP_SCREEN_ON);
	if(keepOn != keepsScreenOn)
	{
		logMsg("keep screen on: %d", keepOn);
		jSetWinFlags(env, jBaseActivity, keepOn ? AWINDOW_FLAG_KEEP_SCREEN_ON : 0, AWINDOW_FLAG_KEEP_SCREEN_ON);
	}
	keepScreenOn = keepOn; // cache value for endIdleByUserActivity()
}

void endIdleByUserActivity()
{
	if(!keepScreenOn)
	{
		//logMsg("signaling user activity to the OS");
		auto env = eEnv();
		// quickly toggle KEEP_SCREEN_ON flag to brighten screen,
		// waiting about 20ms before toggling it back off triggers the screen to brighten if it was already dim
		jSetWinFlags(env, jBaseActivity, AWINDOW_FLAG_KEEP_SCREEN_ON, AWINDOW_FLAG_KEEP_SCREEN_ON);
		userActivityCallback.callbackAfterMSec(
			[]()
			{
				if(!keepScreenOn)
				{
					jSetWinFlags(eEnv(), jBaseActivity, 0, AWINDOW_FLAG_KEEP_SCREEN_ON);
				}
			}, 20);
	}
}

static void setStatusBarHidden(JNIEnv *env, bool hidden)
{
	auto statusBarIsHidden = (bool)(jWinFlags(env, jBaseActivity) & AWINDOW_FLAG_FULLSCREEN);
	if(hidden != statusBarIsHidden)
	{
		logMsg("setting app window fullscreen: %u", hidden);
		jSetWinFlags(env, jBaseActivity, hidden ? AWINDOW_FLAG_FULLSCREEN : 0, AWINDOW_FLAG_FULLSCREEN);
	}
}

void setSysUIStyle(uint flags)
{
	// Flags mapped directly to SYSTEM_UI_FLAG_*
	// SYS_UI_STYLE_DIM_NAV -> SYSTEM_UI_FLAG_LOW_PROFILE (0)
	// SYS_UI_STYLE_HIDE_NAV -> SYSTEM_UI_FLAG_HIDE_NAVIGATION (1)
	// SYS_UI_STYLE_HIDE_STATUS -> SYSTEM_UI_FLAG_FULLSCREEN (2)
	// SYS_UI_STYLE_NO_FLAGS -> SYSTEM_UI_FLAG_VISIBLE (no bits set)

	if(Config::MACHINE_IS_OUYA)
	{
		return;
	}
	auto env = eEnv();
	// Using SYSTEM_UI_FLAG_FULLSCREEN has an odd delay when
	// combined with SYSTEM_UI_FLAG_HIDE_NAVIGATION, so we'll
	// set the window flag even on Android 4.1+.
	// TODO: Re-test on Android versions after 4.4 for any change
	//if(androidSDK() < 16)
	{
		// handle status bar hiding via full-screen window flag
		setStatusBarHidden(env, flags & SYS_UI_STYLE_HIDE_STATUS);
	}
	if(androidSDK() >= 11)
	{
		constexpr uint SYSTEM_UI_FLAG_IMMERSIVE_STICKY = 0x1000;
		// Add SYSTEM_UI_FLAG_IMMERSIVE_STICKY for use with Android 4.4+ if flags contain OS_NAV_STYLE_HIDDEN
		if(flags & SYS_UI_STYLE_HIDE_NAV)
			flags |= SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
		logMsg("setting UI visibility: 0x%X", flags);
		uiVisibilityFlags = flags;;
		jSetUIVisibility(env, jBaseActivity, flags);
	}
}

void setProcessPriority(int nice)
{
	assert(nice > -20);
	logMsg("setting process nice level: %d", nice);
	setpriority(PRIO_PROCESS, 0, nice);
}

bool Screen::supportsFrameTime()
{
	return hasChoreographer;
}

static void onDestroy(ANativeActivity* activity)
{
	::exit(0);
}

static void onStart(ANativeActivity* activity) {}

void onResume(ANativeActivity* activity)
{
	appResumed(activity);
}

static void* onSaveInstanceState(ANativeActivity* activity, size_t* outLen)
{
	return nullptr;
}

void onPause(ANativeActivity* activity)
{
	appPaused();
}

static void onStop(ANativeActivity* activity) {}

static void onConfigurationChanged(ANativeActivity* activity)
{
	AConfiguration_fromAssetManager(aConfig, activity->assetManager);
	configChange(activity->env, aConfig, mainWindow().nWin);
}

static void onLowMemory(ANativeActivity* activity) {}

static void onWindowFocusChanged(ANativeActivity* activity, int focused)
{
	appFocus(focused, mainWindow().nWin);
}

static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window)
{
	finishWindowInit(activity, window, aHasFocus);
}

static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window)
{
	windowDestroyed(activity, mainWindow());
}

static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue)
{
	inputQueue = queue;
	logMsg("input queue created & attached");
	AInputQueue_attachLooper(queue, activityLooper(), ALOOPER_POLL_CALLBACK,
		[](int, int, void* data)
		{
			processInput((AInputQueue*)data);
			return (int)1;
		}, queue);
}

static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue)
{
	logMsg("input queue destroyed");
	inputQueue = nullptr;
	AInputQueue_detachLooper(queue);
}

static void onNativeWindowResized(ANativeActivity* activity, ANativeWindow* window)
{
	windowResized(activity, window);
}

static void onNativeWindowRedrawNeeded(ANativeActivity* activity, ANativeWindow* window)
{
	windowNeedsRedraw(activity, window);
}

static void onContentRectChanged(ANativeActivity* activity, const ARect* rect)
{
	contentRectChanged(activity, *rect, mainWindow().nWin);
}

static void setNativeActivityCallbacks(ANativeActivity* activity)
{
	activity->callbacks->onDestroy = onDestroy;
	//activity->callbacks->onStart = onStart;
	//activity->callbacks->onResume = onResume;
	//activity->callbacks->onSaveInstanceState = onSaveInstanceState;
	//activity->callbacks->onPause = onPause;
	//activity->callbacks->onStop = onStop;
	activity->callbacks->onConfigurationChanged = onConfigurationChanged;
	//activity->callbacks->onLowMemory = onLowMemory;
	activity->callbacks->onWindowFocusChanged = onWindowFocusChanged;
	activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
	activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
	activity->callbacks->onNativeWindowResized = onNativeWindowResized;
	activity->callbacks->onNativeWindowRedrawNeeded = onNativeWindowRedrawNeeded;
	activity->callbacks->onInputQueueCreated = onInputQueueCreated;
	activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
	//activity->callbacks->onContentRectChanged = onContentRectChanged;
}

}

CLINK void LVISIBLE ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize)
{
	using namespace Base;
	logMsg("called ANativeActivity_onCreate, thread ID %d", gettid());
	/*if(aLooper)
	{
		logMsg("skipping onCreate");
		setNativeActivityCallbacks(activity);
		activity->callbacks->onResume = onResume;
		activity->callbacks->onPause = onPause;
		assert(jVM == activity->vm);
		assert(eJEnv == activity->env);
		jBaseActivity = activity->clazz;
		return;
	}*/
	setSDK(activity->sdkVersion);
	if(Base::androidSDK() >= 16)
		processInput = Base::processInputWithGetEvent;
	setupActivityLooper();
	jVM = activity->vm;
	baseActivity = activity;
	jBaseActivity = activity->clazz;
	eJEnv = activity->env;
	activityInit(activity);
	Base::dlLoadFuncs();

	#ifdef CONFIG_INPUT
	doOrAbort(Input::init());
	#endif

	aConfig = AConfiguration_new();
	AConfiguration_fromAssetManager(aConfig, activity->assetManager);
	initConfig(aConfig);
	setNativeActivityCallbacks(activity);
}
