#pragma once

#include <imagine/gfx/opengl/gfx-globals.hh>
#include <imagine/logger/logger.h>

namespace Gfx
{
extern bool checkGLErrors;
extern bool checkGLErrorsVerbose;
}

static const char *glErrorToString(GLenum err)
{
	switch(err)
	{
		case GL_NO_ERROR: return "No Error";
		case GL_INVALID_ENUM: return "Invalid Enum";
		case GL_INVALID_VALUE: return "Invalid Value";
		case GL_INVALID_OPERATION: return "Invalid Operation";
		#if (!defined CONFIG_GFX_OPENGL_ES && defined CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE) \
			|| (defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1)
		case GL_STACK_OVERFLOW: return "Stack Overflow";
		case GL_STACK_UNDERFLOW: return "Stack Underflow";
		#endif
		case GL_OUT_OF_MEMORY: return "Out of Memory";
		default: return "Unknown Error";
	}
}

static const char *glDataTypeToString(int format)
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
		default: bug_branch("%d", format); return NULL;
	}
}

static const char *glImageFormatToString(int format)
{
	switch(format)
	{
		case GL_RGBA8: return "RGBA8";
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
		#if !defined CONFIG_GFX_OPENGL_ES && defined CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		case GL_COMPRESSED_RGBA: return "COMPRESSED_RGBA";
		case GL_COMPRESSED_RGB: return "COMPRESSED_RGB";
		case GL_COMPRESSED_LUMINANCE: return "COMPRESSED_LUMINANCE";
		case GL_COMPRESSED_LUMINANCE_ALPHA: return "COMPRESSED_LUMINANCE_ALPHA";
		#endif
		case GL_LUMINANCE8: return "LUMINANCE8";
		case GL_LUMINANCE8_ALPHA8: return "LUMINANCE8_ALPHA8";
		case GL_ALPHA8: return "ALPHA8";
		case GL_RGBA: return "RGBA";
		case GL_BGRA: return "BGRA";
		case GL_RGB: return "RGB";
		#if defined CONFIG_GFX_OPENGL_ES || defined CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
		case GL_LUMINANCE: return "LUMINANCE";
		case GL_LUMINANCE_ALPHA: return "LUMINANCE_ALPHA";
		#endif
		case GL_ALPHA: return "ALPHA";
		default: bug_branch("%d", format); return NULL;
	}
}

template <class FUNC>
static bool handleGLErrors(FUNC callback)
{
	if(!Gfx::checkGLErrors)
		return false;

	bool gotError = false;
	GLenum error;
	while((error = glGetError()) != GL_NO_ERROR)
	{
		gotError = true;
		callback(error, glErrorToString(error));
	}
	return gotError;
}

static bool handleGLErrors()
{
	return handleGLErrors(
		[](GLenum, const char *errorStr)
		{
			logWarn("clearing error: %s", errorStr);
		});
}

template <class FUNC>
static bool handleGLErrorsVerbose(FUNC callback)
{
	if(!Gfx::checkGLErrorsVerbose)
		return false;
	return handleGLErrors(callback);
}

static bool handleGLErrorsVerbose()
{
	if(!Gfx::checkGLErrorsVerbose)
		return false;
	return handleGLErrors();
}
