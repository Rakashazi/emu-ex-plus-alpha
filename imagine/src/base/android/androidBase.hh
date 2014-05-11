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
extern ANativeActivity *baseActivity;
extern bool framePostedEvent;

Window *deviceWindow();
void androidWindowInitSurface(Window &win, ANativeWindow *nWin);
void androidWindowNeedsRedraw(Window &win);
void androidWindowContentRectChanged(Window &win, const IG::WindowRect &rect, const IG::Point2D<int> &winSize);
void androidWindowSurfaceDestroyed(Window &win);

}
