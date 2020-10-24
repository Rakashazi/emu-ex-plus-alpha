#pragma once
#include <imagine/gfx/opengl/gfx-globals.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/base/GLContext.hh>
#include "utils.h"
#ifdef __ANDROID__
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#if defined CONFIG_BASE_X11 || defined __ANDROID__
#define CONFIG_BASE_GLAPI_EGL
#endif

#ifdef __ANDROID__
extern "C" {
EGLAPI EGLImageKHR EGLAPIENTRY eglCreateImageKHR (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroyImageKHR (EGLDisplay dpy, EGLImageKHR image);
}
#endif

namespace Gfx
{

extern bool checkGLErrors;
extern bool checkGLErrorsVerbose;

static constexpr bool defaultToFullErrorChecks = true;
static constexpr GLuint VATTR_POS = 0, VATTR_TEX_UV = 1, VATTR_COLOR = 2;

static constexpr Base::GLDisplay::API glAPI =
	Config::Gfx::OPENGL_ES ? Base::GLDisplay::API::OPENGL_ES : Base::GLDisplay::API::OPENGL;

Gfx::GC orientationToGC(Base::Orientation o);
void setGLDebugOutput(DrawContextSupport &support, bool on);

#ifdef __ANDROID__
EGLImageKHR makeAndroidNativeBufferEGLImage(EGLDisplay dpy, EGLClientBuffer clientBuff);
#endif

}

#ifndef GL_MIRRORED_REPEAT
#define GL_MIRRORED_REPEAT 0x8370
#endif

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

#ifndef GL_PIXEL_PACK_BUFFER
#define GL_PIXEL_PACK_BUFFER 0x88EB
#endif

#ifndef GL_PIXEL_UNPACK_BUFFER
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif
