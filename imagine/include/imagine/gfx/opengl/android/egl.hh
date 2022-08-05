#pragma once
#include <EGL/egl.h>
#include <EGL/eglext.h>

extern "C" {
EGLAPI EGLImageKHR EGLAPIENTRY eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image);
}

namespace IG::Gfx
{

EGLImageKHR makeAndroidNativeBufferEGLImage(EGLDisplay dpy, EGLClientBuffer clientBuff, bool srgb);

}
