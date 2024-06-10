#pragma once

#if defined __APPLE__
#import <OpenGL/gl3.h>
#import <OpenGL/gl3ext.h>
#else // Generic OpenGL headers
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_PROTOTYPES
#endif

#define IG_GL_API_OGL

#ifndef GL_APIENTRY
#define GL_APIENTRY GLAPIENTRY
#endif

namespace IG
{

constexpr auto glDebugMessageCallbackName = "glDebugMessageCallback";

}
