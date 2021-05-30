#pragma once
#include <imagine/gfx/defs.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/util/Interpolator.hh>

namespace Base
{
class Window;
class ApplicationContext;
}

namespace Gfx
{

struct GLRendererWindowData
{
	constexpr GLRendererWindowData() {}
	Base::GLDrawable drawable{};
	Base::GLBufferConfig bufferConfig{};
	InterpolatorValue<Angle, IG::FrameTime, InterpolatorType::EASEOUTQUAD> projAngleM{};
	Base::GLColorSpace colorSpace{};
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
