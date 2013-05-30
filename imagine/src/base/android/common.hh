#pragma once

#include <config/machine.hh>
#include <util/jni.hh>
#include <util/Motion.hh>
#include <util/branch.h>

extern TimedMotion<GC> projAngleM;
bool glSyncHackEnabled = 0, glSyncHackBlacklisted = 0;
bool glPointerStateHack = 0, glBrokenNpot = 0;

namespace Base
{

namespace Surface
{
	static const int ROTATION_0 = 0, ROTATION_90 = 1, ROTATION_180 = 2, ROTATION_270 = 3;

	static bool isSidewaysOrientation(int o)
	{
		return o == ROTATION_90 || o == ROTATION_270;
	}

	static bool isStraightOrientation(int o)
	{
		return o == ROTATION_0 || o == ROTATION_180;
	}

	static bool orientationsAre90DegreesAway(int o1, int o2)
	{
		switch(o1)
		{
			default: bug_branch("%d", o1);
			case ROTATION_0:
			case ROTATION_180:
				return isSidewaysOrientation(o2);
			case ROTATION_90:
			case ROTATION_270:
				return isStraightOrientation(o2);
		}
	}
}

using namespace Surface;

//static JNIEnv* aJEnv = nullptr;
jclass jBaseActivityCls = nullptr;
jobject jBaseActivity = nullptr;
static JavaInstMethod<void> jSetRequestedOrientation;
static JavaInstMethod<void> jAddNotification, jRemoveNotification;
static jobject vibrator = nullptr;
static jclass vibratorCls = nullptr;
static JavaInstMethod<void> jVibrate;
const char *appPath = nullptr;
static uint aSDK = aMinSDK;
static int osOrientation = -1;
static bool osAnimatesRotation = 0;
static float androidXDPI = 0, androidYDPI = 0, // DPI reported by OS
		xDPI = 0, yDPI = 0; // Active DPI
static float aDensityDPI = 0;
static int aHardKeyboardState = 0, aKeyboardType = ACONFIGURATION_KEYBOARD_NOKEYS, aHasHardKeyboard = 0;
static const char *filesDir = nullptr, *eStoreDir = nullptr;
static bool hasPermanentMenuKey = 1;
static uint visibleScreenY = 1;

static const GC orientationDiffTable[4][4] =
{
		{ 0, -90, 180, 90 },
		{ 90, 0, -90, 180 },
		{ 180, 90, 0, -90 },
		{ -90, 180, 90, 0 },
};

static void setupDPI();

// Public implementation

void exitVal(int returnVal)
{
	logMsg("called exit");
	appState = APP_EXITING;
	//callSafe(onAppExitHandler, onAppExitHandlerCtx, 0);
	onExit(0);
	jRemoveNotification(eEnv(), jBaseActivity);
	logMsg("exiting");
	::exit(returnVal);
}
void abort() { ::abort(); }

void displayNeedsUpdate() { generic_displayNeedsUpdate(); }

//void openURL(const char *url) { };

const char *documentsPath() { return filesDir; }
const char *storagePath() { return eStoreDir; }

bool hasHardwareNavButtons()
{
	return hasPermanentMenuKey;
}

void setDPI(float dpi)
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
	setupDPI();
	Gfx::setupScreenSize();
}

static void setupVibration(JNIEnv* jEnv)
{
	if(!vibratorCls)
	{
		//logMsg("setting up Vibrator class");
		vibratorCls = (jclass)jEnv->NewGlobalRef(jEnv->FindClass("android/os/Vibrator"));
		jVibrate.setup(jEnv, vibratorCls, "vibrate", "(J)V");
	}
}

#if !defined CONFIG_MACHINE_OUYA
bool hasVibrator()
{
	return vibrator;
}

void vibrate(uint ms)
{
	if(unlikely(!vibrator))
		return;
	if(unlikely(!vibratorCls))
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

// Private implementation

void setSDK(uint sdk)
{
	aSDK = sdk;
}

uint androidSDK()
{
	return std::max(aMinSDK, aSDK);
}

static void resizeEvent(const Window &win, bool force = 0)
{
	auto prevTriggerGfxResize = triggerGfxResize;
	generic_resizeEvent(win, force);
	if(prevTriggerGfxResize != triggerGfxResize)
	{
		setupDPI();
	}
}

static void setupDPI()
{
	assert(osOrientation != -1);
	float xdpi = isStraightOrientation(osOrientation) ? xDPI : yDPI;
	float ydpi = isStraightOrientation(osOrientation) ? yDPI : xDPI;
	Gfx::viewMMWidth_ = ((float)mainWin.w / xdpi) * 25.4;
	Gfx::viewMMHeight_ = ((float)mainWin.h / ydpi) * 25.4;
	Gfx::viewSMMWidth_ = (mainWin.w / aDensityDPI) * 25.4;
	Gfx::viewSMMHeight_ = (mainWin.h / aDensityDPI) * 25.4;
	logMsg("calc display size %dx%d MM, scaled %dx%d MM", Gfx::viewMMWidth_, Gfx::viewMMHeight_, Gfx::viewSMMWidth_, Gfx::viewSMMHeight_);
	assert(Gfx::viewMMWidth_ && Gfx::viewMMHeight_);
}

static void initialScreenSizeSetup(uint w, uint h)
{
	mainWin.w = mainWin.rect.x2 = w;
	mainWin.h = mainWin.rect.y2 = h;
	setupDPI();
	if(androidSDK() < 9 && unlikely(Gfx::viewMMWidth_ > 9000)) // hack for Archos Tablets
	{
		logMsg("screen size over 9000! setting to something sane");
		androidXDPI = xDPI = 220;
		androidYDPI = yDPI = 220;
		setupDPI();
	}
}

static void setDeviceType(const char *dev)
{
	assert(Config::ENV_ANDROID_MINSDK >= 4);
	if(Config::MACHINE_IS_GENERIC_ARMV7)
	{
		if(androidSDK() < 11 && (strstr(dev, "shooter") || string_equal(dev, "inc")))
		{
			// Evo 3D/Shooter, & HTC Droid Incredible hack
			logMsg("device needs glFinish() hack");
			glSyncHackBlacklisted = 1;
		}
		else if(androidSDK() < 11 && (string_equal(dev, "vision") || string_equal(dev, "ace")))
		{
			// T-Mobile G2 (HTC Desire Z), HTC Desire HD
			logMsg("device has broken npot support");
			glBrokenNpot = 1;
		}
		else if(androidSDK() < 11 && string_equal(dev, "GT-B5510"))
		{
			logMsg("device needs gl*Pointer() hack");
			glPointerStateHack = 1;
		}
	}
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
		const Input::DeviceChange change = { 0, Input::Event::MAP_KEYBOARD,
				aHardKeyboardState == ACONFIGURATION_KEYSHIDDEN_NO ? Input::DeviceChange::SHOWN : Input::DeviceChange::HIDDEN };
		Input::onInputDevChange(change);
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

#ifdef CONFIG_GFX_SOFT_ORIENTATION
/*CLINK JNIEXPORT void JNICALL LVISIBLE Java_com_imagine_BaseActivity_setOrientation(JNIEnv* env, jobject thiz, jint o)
{
	logMsg("orientation sensor change");
	preferedOrientation = o;
	if(gfx_setOrientation(o))
		postRenderUpdate();
}*/

static bool autoOrientationState = 0; // Defaults to off in Java code

void setAutoOrientation(bool on)
{
	if(autoOrientationState == on)
	{
		//logMsg("auto orientation already in state: %d", (int)on);
		return;
	}
	autoOrientationState = on;
	if(!on)
		preferedOrientation = rotateView;
	logMsg("setting auto-orientation: %s", on ? "on" : "off");
	jSetAutoOrientation(jEnv, (jbyte)on, (jint)rotateView);
}

#else

static bool setOrientationOS(int o)
{
	logMsg("OS orientation change");
	assert(osOrientation != -1);
	if(osOrientation != o)
	{
		/*if((isSidewaysOrientation(osOrientation) && !isSidewaysOrientation(o)) ||
			(!isSidewaysOrientation(osOrientation) && isSidewaysOrientation(o)))*/
		/*if(orientationsAre90DegreesAway(o, osOrientation))
		{
			logMsg("rotating screen DPI");
			IG::swap(androidXDPI, androidYDPI);
			IG::swap(xDPI, yDPI);
			//setupDPI();
			//Gfx::setupScreenSize();
		}*/

		// Close any OS dialog windows since they may have taken
		// the OpenGL context and prevent proper surface resizing
		Input::finishSysTextInput();

		if(!osAnimatesRotation)
		{
			GC rotAngle = orientationDiffTable[osOrientation][o];
			logMsg("animating from %d degrees", (int)rotAngle);
			projAngleM.initLinear(IG::toRadians(rotAngle), 0, 10);
		}
		//logMsg("new value %d", o);
		osOrientation = o;
		return 1;
	}
	return 0;

}

#endif

}

#ifndef CONFIG_GFX_SOFT_ORIENTATION
namespace Gfx
{

uint setOrientation(uint o)
{
	using namespace Base;
	logMsg("requested orientation change to %s", orientationName(o));
	int toSet;
	switch(o)
	{
		default: bug_branch("%d", o);
		case VIEW_ROTATE_AUTO: toSet = -1; // SCREEN_ORIENTATION_UNSPECIFIED
		bcase VIEW_ROTATE_0: toSet = 1; // SCREEN_ORIENTATION_PORTRAIT
		bcase VIEW_ROTATE_90: toSet = 0; // SCREEN_ORIENTATION_LANDSCAPE
		bcase VIEW_ROTATE_180: toSet = androidSDK() > 8 ? 9 : 1; // SCREEN_ORIENTATION_REVERSE_PORTRAIT
		bcase VIEW_ROTATE_270: toSet = androidSDK() > 8 ? 8 : 0; // SCREEN_ORIENTATION_REVERSE_LANDSCAPE
		bcase VIEW_ROTATE_90 | VIEW_ROTATE_270: toSet = androidSDK() > 8 ? 6 : 1; // SCREEN_ORIENTATION_SENSOR_LANDSCAPE
	}
	jSetRequestedOrientation(eEnv(), jBaseActivity, toSet);
	return 1;
}

uint setValidOrientations(uint oMask, bool manageAutoOrientation)
{
	return setOrientation(oMask);
}

}
#endif
