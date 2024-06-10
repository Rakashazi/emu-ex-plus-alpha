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

enum class PresentMode : uint8_t;

struct GLRendererWindowData
{
	constexpr GLRendererWindowData() = default;
	GLDrawable drawable{};
	GLBufferConfig bufferConfig{};
	InterpolatorValue<float, SteadyClockTimePoint, InterpolatorType::EASEOUTQUAD> projAngleM{};
	GLColorSpace colorSpace{};
	int8_t swapInterval{1};
	Rect2<int> viewportRect{};
};

GLRendererWindowData &winData(Window &win);
const GLRendererWindowData &winData(const Window &win);

extern bool checkGLErrors;
extern bool checkGLErrorsVerbose;

constexpr bool forceNoVAOs = false; // for testing non-VAO code paths
constexpr bool defaultToFullErrorChecks = true;
constexpr GLuint VATTR_POS = 0, VATTR_TEX_UV = 1, VATTR_COLOR = 2;

constexpr GL::API glAPI = Config::Gfx::OPENGL_ES ? GL::API::OpenGLES : GL::API::OpenGL;

int toSwapInterval(Window &win, PresentMode mode);

constexpr GLenum asGLType(AttribType type)
{
	switch(type)
	{
		case AttribType::UByte: return GL_UNSIGNED_BYTE;
		case AttribType::Short: return GL_SHORT;
		case AttribType::UShort: return GL_UNSIGNED_SHORT;
		case AttribType::Float: return GL_FLOAT;
	}
	bug_unreachable("invalid AttribType");
}

}
