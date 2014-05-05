#pragma once

#include <imagine/util/jni.hh>
#include <imagine/util/time/sys.hh>
#include "EGLContextHelper.hh"

namespace Base
{

extern JavaInstMethod<void> jSetWinFormat;
extern JavaInstMethod<int> jWinFormat;
extern JavaInstMethod<void> jSetRequestedOrientation;
extern JavaInstMethod<void> jPostFrame, jUnpostFrame;
extern EGLContextHelper eglCtx;
extern EGLDisplay display;
extern int osOrientation;
extern bool resumeAppOnWindowInit;
extern uint onFrameEventIdle;
extern int onFrameEventFd;
extern jobject frameHelper;
extern bool aHasFocus;
extern uint appState;
extern TimeSys orientationEventTime;
extern ANativeActivity *baseActivity;
extern bool framePostedEvent;
Window *deviceWindow();

}
