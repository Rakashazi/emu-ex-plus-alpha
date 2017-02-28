#pragma once

#include <imagine/config/defs.hh>
#include "glIncludes.h"
#include "defs.hh"
#include <imagine/util/normalFloat.hh>
#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/gfx/Mat4.hh>
#include <imagine/base/GLContext.hh>

namespace Gfx
{
class Renderer;

using TransformCoordinate = float;
using VertexPos = float;
using Angle = float;
using TextureCoordinate = float;

static constexpr Angle angleFromDegree(Angle deg) { return IG::radians(deg); }
static constexpr Angle angleFromRadian(Angle rad) { return rad; }
static constexpr Angle angleToDegree(Angle a) { return IG::degrees(a); }
static constexpr Angle angleToRadian(Angle a) { return a; }

static const uint gColor_steps = 255;
using ColorComp = NormalFloat<gColor_steps>;

using TextureRef = GLuint;
using VertexIndex = GLushort;
using VertexColor = uint;
using VertexArrayRef = uint;

static constexpr int TRIANGLE_IMPL = GL_TRIANGLES;
static constexpr int TRIANGLE_STRIP_IMPL = GL_TRIANGLE_STRIP;

static constexpr int ZERO_IMPL = GL_ZERO;
static constexpr int ONE_IMPL = GL_ONE;
static constexpr int SRC_COLOR_IMPL = GL_SRC_COLOR;
static constexpr int ONE_MINUS_SRC_COLOR_IMPL = GL_ONE_MINUS_SRC_COLOR;
static constexpr int DST_COLOR_IMPL = GL_DST_COLOR;
static constexpr int ONE_MINUS_DST_COLOR_IMPL = GL_ONE_MINUS_DST_COLOR;
static constexpr int SRC_ALPHA_IMPL = GL_SRC_ALPHA;
static constexpr int ONE_MINUS_SRC_ALPHA_IMPL = GL_ONE_MINUS_SRC_ALPHA;
static constexpr int DST_ALPHA_IMPL = GL_DST_ALPHA;
static constexpr int ONE_MINUS_DST_ALPHA_IMPL = GL_ONE_MINUS_DST_ALPHA;
static constexpr int CONSTANT_COLOR_IMPL = GL_CONSTANT_COLOR;
static constexpr int ONE_MINUS_CONSTANT_COLOR_IMPL = GL_ONE_MINUS_CONSTANT_COLOR;
static constexpr int CONSTANT_ALPHA_IMPL = GL_CONSTANT_ALPHA;
static constexpr int ONE_MINUS_CONSTANT_ALPHA_IMPL = GL_ONE_MINUS_CONSTANT_ALPHA;

static constexpr auto VertexColorPixelFormat = IG::PIXEL_DESC_ABGR8888;

class VertexInfo
{
public:
	static const uint posOffset = 0;
	static constexpr bool hasColor = false;
	static const uint colorOffset = 0;
	static constexpr bool hasTexture = false;
	static const uint textureOffset = 0;
	template<class Vtx>
	static void bindAttribs(Renderer &r, const Vtx *v);
};

class Vertex : public VertexInfo
{
public:
	VertexPos x,y;

	Vertex() = default;
	Vertex(VertexPos x, VertexPos y):
		x{x}, y{y} {}
	static constexpr uint ID = 1;
};

class ColVertex : public VertexInfo
{
public:
	VertexPos x,y;
	VertexColor color;

	ColVertex() = default;
	constexpr ColVertex(VertexPos x, VertexPos y, uint color = 0):
		x{x}, y{y}, color(color) {}
	static constexpr bool hasColor = true;
	static const uint colorOffset;
	static constexpr uint ID = 2;
};

class TexVertex : public VertexInfo
{
public:
	VertexPos x,y;
	TextureCoordinate u,v;

	TexVertex() = default;
	constexpr TexVertex(VertexPos x, VertexPos y, TextureCoordinate u = 0, TextureCoordinate v = 0):
		x{x}, y{y}, u{u}, v{v} {}
	static constexpr bool hasTexture = true;
	static const uint textureOffset;
	static constexpr uint ID = 3;
};

class ColTexVertex : public VertexInfo
{
public:
	VertexPos x, y;
	TextureCoordinate u, v;
	VertexColor color;

	ColTexVertex() = default;
	constexpr ColTexVertex(VertexPos x, VertexPos y, uint color = 0, TextureCoordinate u = 0, TextureCoordinate v = 0):
		x{x}, y{y}, u{u}, v{v}, color(color) {}
	static constexpr bool hasColor = true;
	static const uint colorOffset;
	static constexpr bool hasTexture = true;
	static const uint textureOffset;
	static constexpr uint ID = 4;
};

using Shader = GLuint;
#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
enum { SHADER_VERTEX = GL_VERTEX_SHADER, SHADER_FRAGMENT = GL_FRAGMENT_SHADER };
#else
enum { SHADER_VERTEX = 1, SHADER_FRAGMENT = 2 }; // dummy values
#endif

class GLSLProgram
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
protected:
	GLuint program_ = 0;

public:
	GLint projectionUniform = -1, modelViewUniform = -1;
	uint projectionUniformAge = 0, modelViewUniformAge = 0;

	GLuint program() { return program_; }

	void initUniforms();
	#endif

public:
	constexpr GLSLProgram() {}
	bool init(Renderer &r, Shader vShader, Shader fShader, bool hasColor, bool hasTex);
	void deinit();
	bool link(Renderer &r);
	explicit operator bool() const
	{
		#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
		return program_;
		#else
		return true;
		#endif
	}
};

class TexProgram : public GLSLProgram
{
public:
	constexpr TexProgram() {}
	void init(Renderer &r, GLuint vShader, GLuint fShader);
};

class ColorProgram : public GLSLProgram
{
public:
	constexpr ColorProgram() {}
	void init(Renderer &r, GLuint vShader, GLuint fShader);
};

// default programs

class DefaultTexReplaceProgram : public TexProgram
{
public:
	constexpr DefaultTexReplaceProgram() {}
	bool compile(Renderer &r);
	void use(Renderer &r) { use(r, nullptr); }
	void use(Renderer &r, Mat4 modelMat) { use(r, &modelMat); }
	void use(Renderer &r, const Mat4 *modelMat);
};

class DefaultTexProgram : public TexProgram
{
public:
	constexpr DefaultTexProgram() {}
	bool compile(Renderer &r);
	void use(Renderer &r) { use(r, nullptr); }
	void use(Renderer &r, Mat4 modelMat) { use(r, &modelMat); }
	void use(Renderer &r, const Mat4 *modelMat);
};

class DefaultTexAlphaReplaceProgram : public TexProgram
{
public:
	DefaultTexProgram *impl{};
	constexpr DefaultTexAlphaReplaceProgram() {}
	bool compile(Renderer &r);
	void use(Renderer &r) { use(r, nullptr); }
	void use(Renderer &r, Mat4 modelMat) { use(r, &modelMat); }
	void use(Renderer &r, const Mat4 *modelMat);
};

class DefaultTexAlphaProgram : public TexProgram
{
public:
	DefaultTexProgram *impl{};
	constexpr DefaultTexAlphaProgram() {}
	bool compile(Renderer &r);
	void use(Renderer &r) { use(r, nullptr); }
	void use(Renderer &r, Mat4 modelMat) { use(r, &modelMat); }
	void use(Renderer &r, const Mat4 *modelMat);
};

class DefaultTexExternalReplaceProgram : public TexProgram
{
public:
	constexpr DefaultTexExternalReplaceProgram() {}
	bool compile(Renderer &r);
	void use(Renderer &r) { use(r, nullptr); }
	void use(Renderer &r, Mat4 modelMat) { use(r, &modelMat); }
	void use(Renderer &r, const Mat4 *modelMat);
};

class DefaultTexExternalProgram : public TexProgram
{
public:
	constexpr DefaultTexExternalProgram() {}
	bool compile(Renderer &r);
	void use(Renderer &r) { use(r, nullptr); }
	void use(Renderer &r, Mat4 modelMat) { use(r, &modelMat); }
	void use(Renderer &r, const Mat4 *modelMat);
};

class DefaultColorProgram : public ColorProgram
{
public:
	constexpr DefaultColorProgram() {}
	bool compile(Renderer &r);
	void use(Renderer &r) { use(r, nullptr); }
	void use(Renderer &r, Mat4 modelMat) { use(r, &modelMat); }
	void use(Renderer &r, const Mat4 *modelMat);
};

using ProgramImpl = GLSLProgram;

enum { TEX_UNSET, TEX_2D_1, TEX_2D_2, TEX_2D_4, TEX_2D_EXTERNAL };

using Drawable = Base::GLDrawable;

}
