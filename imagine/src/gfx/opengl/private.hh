#pragma once
#include <imagine/gfx/opengl/gfx-globals.hh>
#include <imagine/gfx/Gfx.hh>
#include <imagine/base/GLContext.hh>
#include "utils.h"

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

}
