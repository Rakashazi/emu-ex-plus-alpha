#pragma once

#include <imagine/util/jni.hh>
#include <imagine/util/time/sys.hh>
#include <imagine/base/Window.hh>
#include <android/input.h>
#include <android/configuration.h>

namespace Base
{

enum SurfaceRotation : int
{
	SURFACE_ROTATION_0 = 0, SURFACE_ROTATION_90 = 1,
	SURFACE_ROTATION_180 = 2, SURFACE_ROTATION_270 = 3
};

class FrameTimer
{
public:
	virtual void scheduleVSync() = 0;
	virtual void cancel() = 0;
};

extern JavaInstMethod<void> jSetWinFormat;
extern JavaInstMethod<int> jWinFormat;
extern JavaInstMethod<void> jSetRequestedOrientation;
extern SurfaceRotation osRotation;
extern bool aHasFocus;
extern uint appState;
extern AInputQueue *inputQueue;
extern FrameTimer *frameTimer;

Window *deviceWindow();
void androidWindowInitSurface(Window &win, ANativeWindow *nWin);
void androidWindowNeedsRedraw(Window &win);
void androidWindowContentRectChanged(Window &win, const IG::WindowRect &rect, const IG::Point2D<int> &winSize);
void androidWindowSurfaceDestroyed(Window &win);
void initFrameTimer(JNIEnv *env, jobject activity);
void removePostedNotifications();
void initScreens(JNIEnv *env, jobject activity, jclass activityCls);
void handleIntent(JNIEnv *env, jobject activity);
void initActivityLooper();
bool isXperiaPlayDeviceStr(const char *str);

static bool surfaceRotationIsStraight(SurfaceRotation o)
{
	return o == SURFACE_ROTATION_0 || o == SURFACE_ROTATION_180;
}

}

namespace Input
{

class AndroidInputDevice;
extern Device *virtualDev;
extern AndroidInputDevice genericKeyDev;
static constexpr uint maxSysInputDevs = MAX_DEVS;
extern StaticArrayList<AndroidInputDevice*, maxSysInputDevs> sysInputDev;
extern bool handleVolumeKeys;
extern bool allowOSKeyRepeats;
extern void (*processInput)(AInputQueue *inputQueue);

void setEventsUseOSInputMethod(bool on);
bool eventsUseOSInputMethod();
void initInputConfig(AConfiguration* config);
void changeInputConfig(AConfiguration *config);
bool hasHardKeyboard();
int hardKeyboardState();
int keyboardType();
void processInputWithGetEvent(AInputQueue *inputQueue);
void processInputWithHasEvents(AInputQueue *inputQueue);
void onPauseMOGA(JNIEnv *env);
void onResumeMOGA(JNIEnv *env, bool notify);
bool dlLoadAndroidFuncs(void *libandroid);

}
