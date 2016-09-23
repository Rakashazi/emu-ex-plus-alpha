#pragma once

#define GLX_GLXEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#define BOOL X11BOOL
#include <GL/glx.h>
#include <GL/glxext.h>
#undef BOOL
#undef GL_GLEXT_PROTOTYPES
#undef GLX_GLXEXT_PROTOTYPES
