#pragma once

#include <imagine/util/jni.hh>
#include <android/input.h>
#include <android/configuration.h>
#include <vector>
#include <memory>

struct ANativeActivity;

namespace Base
{

class Window;
class Screen;
class ApplicationContext;

enum SurfaceRotation : int
{
	SURFACE_ROTATION_0 = 0, SURFACE_ROTATION_90 = 1,
	SURFACE_ROTATION_180 = 2, SURFACE_ROTATION_270 = 3
};

class FrameTimer;

extern JavaInstMethod<void(jint)> jSetRequestedOrientation;
extern SurfaceRotation osRotation;
extern AInputQueue *inputQueue;
extern std::unique_ptr<FrameTimer> frameTimer;

Window *deviceWindow(ApplicationContext);
void androidWindowNeedsRedraw(Window &win, bool sync = true);
void initFrameTimer(ApplicationContext, Screen &);
void removePostedNotifications(ApplicationContext);
void initScreens(ApplicationContext, JNIEnv *, jobject activity, jclass activityCls);
void handleIntent(ANativeActivity *);
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
extern void (*processInput)(Base::ApplicationContext, AInputQueue *);

void init(Base::ApplicationContext, JNIEnv *);
void setEventsUseOSInputMethod(bool on);
bool eventsUseOSInputMethod();
void initInputConfig(AConfiguration* config);
void changeInputConfig(AConfiguration *config);
bool hasHardKeyboard();
int hardKeyboardState();
int keyboardType();
void processInputWithGetEvent(Base::ApplicationContext, AInputQueue *);
void processInputWithHasEvents(Base::ApplicationContext, AInputQueue *);
bool hasGetAxisValue();
bool addInputDevice(AndroidInputDevice dev, bool updateExisting, bool notify);
bool removeInputDevice(int id, bool notify);

}
