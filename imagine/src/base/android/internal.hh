#pragma once

#include <imagine/util/jni.hh>
#include <imagine/util/time/sys.hh>
#include <imagine/base/Window.hh>
#include <android/input.h>
#include <android/configuration.h>

namespace Base
{

class FrameTimer
{
public:
	virtual void scheduleVSync() = 0;
	virtual void cancel() = 0;
};

extern JavaInstMethod<void> jSetWinFormat;
extern JavaInstMethod<int> jWinFormat;
extern JavaInstMethod<void> jSetRequestedOrientation;
extern int osOrientation;
extern bool aHasFocus;
extern uint appState;
extern AInputQueue *inputQueue;
extern void (*processInput)(AInputQueue *inputQueue);
extern FrameTimer *frameTimer;

Window *deviceWindow();
void androidWindowInitSurface(Window &win, ANativeWindow *nWin);
void androidWindowNeedsRedraw(Window &win);
void androidWindowContentRectChanged(Window &win, const IG::WindowRect &rect, const IG::Point2D<int> &winSize);
void androidWindowSurfaceDestroyed(Window &win);
void initFrameTimer(JNIEnv *jEnv, jobject activity);
void removePostedNotifications();
void initScreens(JNIEnv *jEnv, jobject activity, jclass activityCls);
void initInputConfig(AConfiguration* config);
void changeInputConfig(JNIEnv *jEnv, AConfiguration *config);
void processInputWithGetEvent(AInputQueue *inputQueue);
void handleIntent(ANativeActivity* activity);
void initActivityLooper();

}
