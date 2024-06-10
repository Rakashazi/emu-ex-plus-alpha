#pragma once

#if defined __APPLE__
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#else // Generic OpenGL ES headers
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#undef GL_GLEXT_PROTOTYPES
#endif

#define IG_GL_API_OGL_ES

// Symbol Re-mapping

#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif
#ifndef GL_ALPHA8
#define GL_ALPHA8 0x803C
#endif
#ifndef GL_LUMINANCE8
#define GL_LUMINANCE8 0x8040
#endif
#ifndef GL_LUMINANCE8_ALPHA8
#define GL_LUMINANCE8_ALPHA8 0x8045
#endif
#ifndef GL_SYNC_FLUSH_COMMANDS_BIT
#define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
#endif

#ifndef GL_KHR_debug
typedef void (GL_APIENTRY *GLDEBUGPROCKHR)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void *);
#define GL_DEBUG_OUTPUT_KHR 0x92E0
#endif

#ifndef GL_DEBUG_OUTPUT
#define GL_DEBUG_OUTPUT GL_DEBUG_OUTPUT_KHR
#endif

using GLDEBUGPROC = GLDEBUGPROCKHR;

namespace IG
{

constexpr auto glDebugMessageCallbackName = "glDebugMessageCallbackKHR";

}
