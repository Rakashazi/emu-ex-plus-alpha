#pragma once

#include <stdint.h>

#if CONFIG_ENV_ANDROID_MINSDK >= 9
#error use the NDK EGL headers
#endif

#ifdef __cplusplus
extern "C" {
#endif

// EGL/eglplatform.h
typedef void*                           EGLNativeDisplayType;
typedef int32_t EGLint;

// EGL/egl.h

typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef void *EGLContext;
typedef void *EGLDisplay;
typedef void *EGLClientBuffer;
#define EGL_NO_CONTEXT                  ((EGLContext)0)
#define EGL_NO_DISPLAY                  ((EGLDisplay)0)
#define EGL_DEFAULT_DISPLAY             ((EGLNativeDisplayType)0)
#define EGL_FALSE			0
#define EGL_TRUE			1
#define EGL_NONE			0x3038	/* Attrib list terminator */
#define EGL_EXTENSIONS      0x3055

// EGL/eglext.h

typedef void *EGLImageKHR;
#define EGL_NO_IMAGE_KHR                        ((EGLImageKHR)0)
#define EGL_NATIVE_BUFFER_ANDROID       0x3140  /* eglCreateImageKHR target */

#define EGL_IMAGE_PRESERVED_KHR			0x30D2	/* eglCreateImageKHR attribute */

#ifdef __cplusplus
}
#endif
