#pragma once

#include <gfx/opengl/glIncludes.h>

static const char *glErrorToString(GLenum err)
{
	switch(err)
	{
		case GL_NO_ERROR: return "No Error";
		case GL_INVALID_ENUM: return "Invalid Enum";
		case GL_INVALID_VALUE: return "Invalid Value";
		case GL_INVALID_OPERATION: return "Invalid Operation";
		case GL_STACK_OVERFLOW: return "Stack Overflow";
		case GL_STACK_UNDERFLOW: return "Stack Underflow";
		case GL_OUT_OF_MEMORY: return "Out of Memory";
		default: return "Unknown Error";
	}
}

static const char *glDataTypeToString(int format)
{
	switch(format)
	{
		case GL_UNSIGNED_BYTE: return "B";
		#if !defined(CONFIG_GFX_OPENGL_ES) || defined(CONFIG_BASE_PS3)
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
		#if defined(CONFIG_BASE_PS3)
		case GL_ARGB_SCE: return "ARGB_SCE";
		#endif
		#if !defined(CONFIG_GFX_OPENGL_ES) || defined(CONFIG_BASE_PS3)
		case GL_RGBA8: return "RGBA8";
		case GL_RGB8: return "RGB8";
		case GL_RGB5_A1: return "RGB5_A1";
		case GL_RGB5: return "RGB5";
		case GL_RGBA4: return "RGBA4";
		case GL_BGR: return "BGR";
		case GL_LUMINANCE8: return "LUMINANCE8";
		case GL_LUMINANCE8_ALPHA8: return "LUMINANCE8_ALPHA8";
		#endif
		#if !defined(CONFIG_GFX_OPENGL_ES)
		case GL_COMPRESSED_RGBA: return "COMPRESSED_RGBA";
		case GL_COMPRESSED_RGB: return "COMPRESSED_RGB";
		case GL_COMPRESSED_LUMINANCE: return "COMPRESSED_LUMINANCE";
		case GL_COMPRESSED_LUMINANCE_ALPHA: return "COMPRESSED_LUMINANCE_ALPHA";
		#endif
		case GL_RGBA: return "RGBA";
		case GL_BGRA: return "BGRA";
		case GL_RGB: return "RGB";
		case GL_LUMINANCE: return "LUMINANCE";
		case GL_LUMINANCE_ALPHA: return "LUMINANCE_ALPHA";
		default: bug_branch("%d", format); return NULL;
	}
}

// linker errors on some error check code due to GCC <= 4.7.2 lambda bug
#if defined NDEBUG || (!defined __clang__ && GCC_VERSION < 40703)
	static const bool checkGLErrors = 0;
#else
	static const bool checkGLErrors = 1;
#endif

static const bool checkGLErrorsVerbose = 1;

static bool handleGLErrors(void (*callback)(GLenum error, const char *str) = nullptr)
{
	if(!checkGLErrors)
		return 0;

	bool gotError = 0;
	GLenum error;
	while((error = glGetError()) != GL_NO_ERROR)
	{
		gotError = 1;
		if(callback)
			callback(error, glErrorToString(error));
		else
			logWarn("clearing error: %s", glErrorToString(error));
	}
	return gotError;
}

static bool handleGLErrorsVerbose(void (*callback)(GLenum error, const char *str) = nullptr)
{
	if(!checkGLErrorsVerbose)
		return 0;
	return handleGLErrors(callback);
}
