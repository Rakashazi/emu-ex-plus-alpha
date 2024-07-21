#pragma once

#include <imagine/gfx/opengl/defs.hh>
#include <imagine/logger/logger.h>

#ifndef GL_RGB8
#define GL_RGB8 0x8051
#endif

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

#ifndef GL_LUMINANCE
#define GL_LUMINANCE 0x1909
#endif

#ifndef GL_LUMINANCE_ALPHA
#define GL_LUMINANCE_ALPHA 0x190A
#endif

#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif

#ifndef GL_R8
#define GL_R8 0x8229
#endif

#ifndef GL_RG
#define GL_RG 0x8227
#endif

#ifndef GL_RG8
#define GL_RG8 0x822B
#endif

#ifndef GL_RED
#define GL_RED 0x1903
#endif

#ifndef GL_GREEN
#define GL_GREEN 0x1904
#endif

#ifndef GL_BLUE
#define GL_BLUE 0x1905
#endif

#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8 0x8C43
#endif

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

namespace IG::Gfx
{
extern bool checkGLErrors;
extern bool checkGLErrorsVerbose;
}

constexpr const char *glErrorToString(GLenum err)
{
	switch(err)
	{
		case GL_NO_ERROR: return "No Error";
		case GL_INVALID_ENUM: return "Invalid Enum";
		case GL_INVALID_VALUE: return "Invalid Value";
		case GL_INVALID_OPERATION: return "Invalid Operation";
		case GL_OUT_OF_MEMORY: return "Out of Memory";
		default: return "Unknown Error";
	}
}

constexpr const char *glDataTypeToString(int format)
{
	switch(format)
	{
		case GL_UNSIGNED_BYTE: return "B";
		#if !defined CONFIG_GFX_OPENGL_ES
		case GL_UNSIGNED_INT_8_8_8_8: return "I8888";
		case GL_UNSIGNED_INT_8_8_8_8_REV: return "I8888R";
		case GL_UNSIGNED_SHORT_1_5_5_5_REV: return "S1555";
		case GL_UNSIGNED_SHORT_4_4_4_4_REV: return "S4444R";
		#endif
		case GL_UNSIGNED_SHORT_5_6_5: return "S565";
		case GL_UNSIGNED_SHORT_5_5_5_1: return "S5551";
		case GL_UNSIGNED_SHORT_4_4_4_4: return "S4444";
		default: bug_unreachable("fortmat == %d", format);
	}
}

constexpr const char *glImageFormatToString(int format)
{
	switch(format)
	{
		case GL_RGBA8: return "RGBA8";
		case GL_SRGB8_ALPHA8: return "SRGB8A8";
		case GL_RGB8: return "RGB8";
		case GL_RGB5_A1: return "RGB5_A1";
		#if defined CONFIG_GFX_OPENGL_ES
		case GL_RGB565: return "RGB565";
		#else
		case GL_RGB5: return "RGB5";
		#endif
		case GL_RGBA4: return "RGBA4";
		case GL_BGR: return "BGR";
		case GL_RED: return "RED";
		case GL_R8: return "R8";
		case GL_RG: return "RG";
		case GL_RG8: return "RG8";
		case GL_LUMINANCE8: return "LUMINANCE8";
		case GL_LUMINANCE8_ALPHA8: return "LUMINANCE8_ALPHA8";
		case GL_LUMINANCE_ALPHA: return "LUMINANCE_ALPHA";
		case GL_ALPHA8: return "ALPHA8";
		case GL_RGBA: return "RGBA";
		case GL_BGRA: return "BGRA";
		case GL_RGB: return "RGB";
		case GL_ALPHA: return "ALPHA";
		default: bug_unreachable("format == %d", format);
	}
}

template <class FUNC>
static bool handleGLErrors2(FUNC callback)
{
	bool gotError = false;
	GLenum error;
	while((error = glGetError()) != GL_NO_ERROR)
	{
		gotError = true;
		callback(error, glErrorToString(error));
	}
	return gotError;
}

static bool handleGLErrors2()
{
	return handleGLErrors2(
		[](GLenum, const char *errorStr)
		{
			logWarn("clearing error: %s", errorStr);
		});
}

template <class FUNC>
static bool runGLCheckedAlways(FUNC func, const char *label = nullptr)
{
	handleGLErrors2();
	func();
	return !handleGLErrors2(
		[label](GLenum, const char *err)
		{
			if(label)
			{
				logErr("%s in %s", err, label);
			}
			else
			{
				logErr("%s", err);
			}
		});
}

template <class FUNC>
static bool runGLChecked(FUNC func, const char *label = nullptr)
{
	if(!IG::Gfx::checkGLErrors)
	{
		func();
		return true;
	}
	return runGLCheckedAlways(func, label);
}

template <class FUNC>
static bool runGLCheckedVerbose(FUNC func, const char *label = nullptr)
{
	if(!IG::Gfx::checkGLErrorsVerbose)
	{
		func();
		return true;
	}
	return runGLChecked(func, label);
}

inline GLuint makeGLTextureName(GLuint oldTex)
{
	if(oldTex)
	{
		glDeleteTextures(1, &oldTex);
	}
	GLuint tex;
	glGenTextures(1, &tex);
	return tex;
}
