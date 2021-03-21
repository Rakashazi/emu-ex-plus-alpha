#pragma once
#include <imagine/gfx/defs.hh>
#include <imagine/base/GLContext.hh>
#include "../common/DrawableHolder.hh"
#include <imagine/util/Interpolator.hh>

namespace Base
{
class Window;
}

namespace Gfx
{

struct GLRendererWindowData
{
	Gfx::DrawableHolder drawableHolder{};
	InterpolatorValue<Angle, IG::FrameTime, InterpolatorType::EASEOUTQUAD> projAngleM{};
};

GLRendererWindowData &winData(Base::Window &win);

extern bool checkGLErrors;
extern bool checkGLErrorsVerbose;

static constexpr bool defaultToFullErrorChecks = true;
static constexpr GLuint VATTR_POS = 0, VATTR_TEX_UV = 1, VATTR_COLOR = 2;

static constexpr Base::GL::API glAPI =
	Config::Gfx::OPENGL_ES ? Base::GL::API::OPENGL_ES : Base::GL::API::OPENGL;

Gfx::GC orientationToGC(Base::Orientation o);

}
