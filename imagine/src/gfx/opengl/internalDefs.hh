#pragma once
#include <imagine/gfx/defs.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/util/Interpolator.hh>

namespace IG
{
class Window;
class ApplicationContext;
}

namespace IG::Gfx
{

struct GLRendererWindowData
{
	constexpr GLRendererWindowData() = default;
	GLDrawable drawable{};
	GLBufferConfig bufferConfig{};
	InterpolatorValue<float, IG::FrameTime, InterpolatorType::EASEOUTQUAD> projAngleM{};
	GLColorSpace colorSpace{};
	Rect2<int> viewportRect{};
};

GLRendererWindowData &winData(Window &win);

extern bool checkGLErrors;
extern bool checkGLErrorsVerbose;

static constexpr bool defaultToFullErrorChecks = true;
static constexpr GLuint VATTR_POS = 0, VATTR_TEX_UV = 1, VATTR_COLOR = 2;

static constexpr GL::API glAPI =
	Config::Gfx::OPENGL_ES ? GL::API::OPENGL_ES : GL::API::OPENGL;

float orientationRadians(Orientation o);

}
