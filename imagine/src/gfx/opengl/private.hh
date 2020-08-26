#pragma once
#include <imagine/gfx/opengl/gfx-globals.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/base/GLContext.hh>
#include "utils.h"
#ifdef __ANDROID__
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>
#endif

#if defined CONFIG_BASE_X11 || defined __ANDROID__
#define CONFIG_BASE_GLAPI_EGL
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
GLuint defineEGLImageTexture(Renderer &r, EGLImageKHR eglImg, GLuint tex);
#endif

}
