#pragma once

#include <imagine/util/jni.hh>
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

extern JavaInstMethod<void(jint)> jSetWinFormat;
extern JavaInstMethod<jint()> jWinFormat;
extern JavaInstMethod<void(jint)> jSetRequestedOrientation;
extern SurfaceRotation osRotation;
extern uint appState;
extern AInputQueue *inputQueue;
extern FrameTimer *frameTimer;

Window *deviceWindow();
void androidWindowNeedsRedraw(Window &win);
void initFrameTimer(JNIEnv *env, jobject activity);
void removePostedNotifications();
void initScreens(JNIEnv *env, jobject activity, jclass activityCls);
void handleIntent(JNIEnv *env, jobject activity);
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
bool hasGetAxisValue();

}

#if __ANDROID_API__ < 12
extern float (*AMotionEvent_getAxisValue)(const AInputEvent* motion_event, int32_t axis, size_t pointer_index);
#endif
