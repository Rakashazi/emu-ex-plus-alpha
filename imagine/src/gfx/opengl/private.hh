#pragma once
#include <imagine/gfx/opengl/gfx-globals.hh>
#include <imagine/gfx/Gfx.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/gfx/Texture.hh>
#include "utils.h"

namespace Gfx
{

extern bool checkGLErrors;
extern bool checkGLErrorsVerbose;

static constexpr bool defaultToFullErrorChecks = true;
static constexpr GLuint VATTR_POS = 0, VATTR_TEX_UV = 1, VATTR_COLOR = 2;

Gfx::GC orientationToGC(uint o);
void setGLDebugOutput(DrawContextSupport &support, bool on);

}
