#pragma once

#include <imagine/engine-globals.h>
#include <imagine/util/fixed.hh>
#include <imagine/util/normalFloat.hh>
#include <imagine/util/pixel.h>

#if defined __APPLE__ && !defined __ARM_ARCH_6K__
#define CONFIG_GFX_MATH_GLKIT
#else
#define CONFIG_GFX_MATH_GLM
#endif

#include <imagine/gfx/Mat4.hh>
#include "glIncludes.h"

namespace Config
{
	namespace Gfx
	{
	#if !defined CONFIG_BASE_MACOSX && \
	((defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1) || !defined CONFIG_GFX_OPENGL_ES)
	#define CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	static constexpr bool OPENGL_FIXED_FUNCTION_PIPELINE = true;
	#else
	static constexpr bool OPENGL_FIXED_FUNCTION_PIPELINE = false;
	#endif

	#if (defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 2) || !defined CONFIG_GFX_OPENGL_ES
	#define CONFIG_GFX_OPENGL_SHADER_PIPELINE
	static constexpr bool OPENGL_SHADER_PIPELINE = true;
	#else
	static constexpr bool OPENGL_SHADER_PIPELINE = false;
	#endif

	#if !defined CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE && !defined CONFIG_GFX_OPENGL_SHADER_PIPELINE
	#error "Configuration error, OPENGL_FIXED_FUNCTION_PIPELINE & OPENGL_SHADER_PIPELINE both unset"
	#endif

	#ifdef CONFIG_GFX_OPENGL_ES_MAJOR_VERSION
	static constexpr bool OPENGL_ES = true;
	static constexpr int OPENGL_ES_MAJOR_VERSION = CONFIG_GFX_OPENGL_ES_MAJOR_VERSION;
	#else
	static constexpr bool OPENGL_ES = false;
	#define CONFIG_GFX_OPENGL_ES_MAJOR_VERSION 0
	static constexpr int OPENGL_ES_MAJOR_VERSION = 0;
	#endif

	#ifdef __ANDROID__
	#define CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
	#endif
	}
}

namespace Gfx
{
using TransformCoordinate = float;
using VertexPos = float;
using Angle = float;
using TextureCoordinate = float;

static constexpr Angle angleFromDegree(Angle deg) { return IG::toRadians(deg); }
static constexpr Angle angleFromRadian(Angle rad) { return rad; }
static constexpr Angle angleToDegree(Angle a) { return IG::toDegrees(a); }
static constexpr Angle angleToRadian(Angle a) { return a; }

static const uint gColor_steps = 255;
using ColorComp = NormalFloat<gColor_steps>;

using TextureRef = GLuint;
using VertexIndex = GLushort;
using VertexColor = uint;
using VertexArrayRef = uint;

static const PixelFormatDesc &VertexColorPixelFormat = PixelFormatABGR8888;

class VertexInfo
{
public:
	static const uint posOffset = 0;
	static constexpr bool hasColor = false;
	static const uint colorOffset = 0;
	static constexpr bool hasTexture = false;
	static const uint textureOffset = 0;
	template<class Vtx>
	static void draw(const Vtx *v, uint type, uint count);
	template<class Vtx>
	static void draw(const Vtx *v, const VertexIndex *idx, uint type, uint count);
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
	bool init(Shader vShader, Shader fShader, bool hasColor, bool hasTex);
	void deinit();
	bool link();
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
	void init(GLuint vShader, GLuint fShader);
};

class ColorProgram : public GLSLProgram
{
public:
	constexpr ColorProgram() {}
	void init(GLuint vShader, GLuint fShader);
};

// default programs

class DefaultTexReplaceProgram : public TexProgram
{
public:
	constexpr DefaultTexReplaceProgram() {}
	bool compile();
	void use() { use(nullptr); }
	void use(Mat4 modelMat) { use(&modelMat); }
	void use(const Mat4 *modelMat);
};

class DefaultTexProgram : public TexProgram
{
public:
	constexpr DefaultTexProgram() {}
	bool compile();
	void use() { use(nullptr); }
	void use(Mat4 modelMat) { use(&modelMat); }
	void use(const Mat4 *modelMat);
};

class DefaultTexAlphaReplaceProgram : public TexProgram
{
public:
	DefaultTexProgram *impl{};
	constexpr DefaultTexAlphaReplaceProgram() {}
	bool compile();
	void use() { use(nullptr); }
	void use(Mat4 modelMat) { use(&modelMat); }
	void use(const Mat4 *modelMat);
};

class DefaultTexAlphaProgram : public TexProgram
{
public:
	DefaultTexProgram *impl{};
	constexpr DefaultTexAlphaProgram() {}
	bool compile();
	void use() { use(nullptr); }
	void use(Mat4 modelMat) { use(&modelMat); }
	void use(const Mat4 *modelMat);
};

class DefaultTexExternalReplaceProgram : public TexProgram
{
public:
	constexpr DefaultTexExternalReplaceProgram() {}
	bool compile();
	void use() { use(nullptr); }
	void use(Mat4 modelMat) { use(&modelMat); }
	void use(const Mat4 *modelMat);
};

class DefaultTexExternalProgram : public TexProgram
{
public:
	constexpr DefaultTexExternalProgram() {}
	bool compile();
	void use() { use(nullptr); }
	void use(Mat4 modelMat) { use(&modelMat); }
	void use(const Mat4 *modelMat);
};

class DefaultColorProgram : public ColorProgram
{
public:
	constexpr DefaultColorProgram() {}
	bool compile();
	void use() { use(nullptr); }
	void use(Mat4 modelMat) { use(&modelMat); }
	void use(const Mat4 *modelMat);
};

// color replacement
extern DefaultTexReplaceProgram texReplaceProgram;
extern DefaultTexAlphaReplaceProgram texAlphaReplaceProgram;
extern DefaultTexReplaceProgram &texIntensityAlphaReplaceProgram;
extern DefaultTexExternalReplaceProgram texExternalReplaceProgram;

// color modulation
extern DefaultTexProgram texProgram;
extern DefaultTexAlphaProgram texAlphaProgram;
extern DefaultTexProgram &texIntensityAlphaProgram;
extern DefaultTexExternalProgram texExternalProgram;
extern DefaultColorProgram noTexProgram;

using ProgramImpl = GLSLProgram;

enum { TEX_UNSET, TEX_2D_1, TEX_2D_2, TEX_2D_4, TEX_2D_EXTERNAL };

}
