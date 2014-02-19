#pragma once

#include <engine-globals.h>
#include <util/fixed.hh>
#include <util/normalFloat.hh>

#if defined __APPLE__ && (defined __i386__ || __ARM_ARCH >= 7)
#define CONFIG_GFX_MATH_GLKIT
#else
#define CONFIG_GFX_MATH_GLM
#endif

#include <gfx/Mat4.hh>
#include "glIncludes.h"

namespace Gfx
{
/*#ifdef CONFIG_BASE_ANDROID
	typedef Fixed16S16 TransformCoordinate;
	typedef Fixed16S16 Angle;
	typedef Fixed16S16 VertexPos;
	#define GL_VERT_ARRAY_TYPE GL_FIXED
	typedef Fixed16S16 TextureCoordinate;
	#define GL_TEX_ARRAY_TYPE GL_FIXED
#else*/
	typedef float TransformCoordinate;
	typedef float VertexPos;
	typedef float Angle;
	typedef float TextureCoordinate;
//#endif

static constexpr Angle angleFromDegree(Angle deg) { return IG::toRadians(deg); }
static constexpr Angle angleFromRadian(Angle rad) { return rad; }
static constexpr Angle angleToDegree(Angle a) { return IG::toDegrees(a); }
static constexpr Angle angleToRadian(Angle a) { return a; }

static const uint gColor_steps = 255;
typedef NormalFloat<gColor_steps> ColorComp;
//typedef NormalFloat<gColor_steps> GColorD;
//typedef NormalFixed<int32, 16, int64, gColor_steps> GColor;
//typedef NormalFixed<int32, 16, int64, gColor_steps> GColorD;

typedef GLuint TextureHandle;
typedef GLushort VertexIndex;
typedef uint VertexColor;
typedef uint VertexArrayRef;

//#ifdef CONFIG_BASE_PS3
//#define VertexColorPixelFormat PixelFormatRGBA8888
//#else
#define VertexColorPixelFormat PixelFormatABGR8888
//#endif

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

class TextureDesc
{
public:
	TextureHandle tid = 0;
	#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
	GLenum target = GL_TEXTURE_2D;
	#else
	static const GLenum target = GL_TEXTURE_2D;
	#endif
	TextureCoordinate xStart = 0, xEnd = 0;
	TextureCoordinate yStart = 0, yEnd = 0;

	constexpr TextureDesc() {}
};

uint maxOpenGLMajorVersionSupport();

using Shader = GLuint;
#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
enum { SHADER_VERTEX = GL_VERTEX_SHADER, SHADER_FRAGMENT = GL_FRAGMENT_SHADER };
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
	operator bool() const
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
	DefaultTexProgram *impl = nullptr;
	constexpr DefaultTexAlphaReplaceProgram() {}
	bool compile();
	void use() { use(nullptr); }
	void use(Mat4 modelMat) { use(&modelMat); }
	void use(const Mat4 *modelMat);
};

class DefaultTexAlphaProgram : public TexProgram
{
public:
	DefaultTexProgram *impl = nullptr;
	constexpr DefaultTexAlphaProgram() {}
	bool compile();
	void use() { use(nullptr); }
	void use(Mat4 modelMat) { use(&modelMat); }
	void use(const Mat4 *modelMat);
};

class DefaultTexIntensityAlphaReplaceProgram : public TexProgram
{
public:
	DefaultTexReplaceProgram *impl = nullptr;
	constexpr DefaultTexIntensityAlphaReplaceProgram() {}
	bool compile();
	void use() { use(nullptr); }
	void use(Mat4 modelMat) { use(&modelMat); }
	void use(const Mat4 *modelMat);
};

class DefaultTexIntensityAlphaProgram : public TexProgram
{
public:
	DefaultTexProgram *impl = nullptr;
	constexpr DefaultTexIntensityAlphaProgram() {}
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
extern DefaultTexIntensityAlphaReplaceProgram texIntensityAlphaReplaceProgram;
extern DefaultTexExternalReplaceProgram texExternalReplaceProgram;

// color modulation
extern DefaultTexProgram texProgram;
extern DefaultTexAlphaProgram texAlphaProgram;
extern DefaultTexIntensityAlphaProgram texIntensityAlphaProgram;
extern DefaultTexExternalProgram texExternalProgram;
extern DefaultColorProgram noTexProgram;

using ProgramImpl = GLSLProgram;

enum { TEX_UNSET, TEX_2D_1, TEX_2D_2, TEX_2D_4, TEX_2D_EXTERNAL };

}
