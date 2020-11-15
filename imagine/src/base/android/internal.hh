#pragma once

#include <imagine/util/jni.hh>
#include <android/input.h>
#include <android/configuration.h>
#include <vector>
#include <memory>

namespace Base
{

class Window;
class Screen;

enum SurfaceRotation : int
{
	SURFACE_ROTATION_0 = 0, SURFACE_ROTATION_90 = 1,
	SURFACE_ROTATION_180 = 2, SURFACE_ROTATION_270 = 3
};

class FrameTimer;

extern JavaInstMethod<void(jint)> jSetWinFormat;
extern JavaInstMethod<jint()> jWinFormat;
extern JavaInstMethod<void(jint)> jSetRequestedOrientation;
extern SurfaceRotation osRotation;
extern AInputQueue *inputQueue;
extern std::unique_ptr<FrameTimer> frameTimer;

Window *deviceWindow();
void androidWindowNeedsRedraw(Window &win, bool sync = true);
void initFrameTimer(JNIEnv *env, jobject activity, Screen &screen);
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
extern const AndroidInputDevice *virtualDev;
extern std::vector<std::unique_ptr<AndroidInputDevice>> sysInputDev;
extern void (*processInput)(AInputQueue *inputQueue);

void init(JNIEnv *env);
void setEventsUseOSInputMethod(bool on);
bool eventsUseOSInputMethod();
void initInputConfig(AConfiguration* config);
void changeInputConfig(AConfiguration *config);
bool hasHardKeyboard();
int hardKeyboardState();
int keyboardType();
void processInputWithGetEvent(AInputQueue *inputQueue);
void processInputWithHasEvents(AInputQueue *inputQueue);
bool hasGetAxisValue();
bool addInputDevice(AndroidInputDevice dev, bool updateExisting, bool notify);
bool removeInputDevice(int id, bool notify);

}
