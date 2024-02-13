#pragma once

#include "defs.hh"

// Header Locations For Platform

#if defined CONFIG_OS_IOS
	#if CONFIG_GFX_OPENGL_ES == 1
	#import <OpenGLES/ES1/gl.h>
	#import <OpenGLES/ES1/glext.h>
	typedef char GLchar;
	#else
	#import <OpenGLES/ES2/gl.h>
	#import <OpenGLES/ES2/glext.h>
	#endif
#elif defined CONFIG_BASE_MACOSX
#import <OpenGL/gl3.h>
#import <OpenGL/gl3ext.h>
#elif defined CONFIG_BASE_WIN32
#include <util/windows/windows.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/wglew.h>
#elif defined CONFIG_GFX_OPENGL_ES // Generic OpenGL ES headers
#define GL_GLEXT_PROTOTYPES
	#if CONFIG_GFX_OPENGL_ES == 1
	#include <GLES/gl.h>
	#include <GLES/glext.h>
	#else
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
	#endif
#undef GL_GLEXT_PROTOTYPES
#else // Generic OpenGL headers
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#undef GL_GLEXT_PROTOTYPES
#endif

// Symbol Re-mapping

#if defined CONFIG_GFX_OPENGL_ES
	#ifndef GL_BGRA
	#define GL_BGRA GL_BGRA_EXT
	#endif
	#ifndef GL_ALPHA8
	#define GL_ALPHA8 0x803C
	#endif
	#ifndef GL_LUMINANCE8
	#define GL_LUMINANCE8 0x8040
	#endif
	#ifndef GL_LUMINANCE8_ALPHA8
	#define GL_LUMINANCE8_ALPHA8 0x8045
	#endif
	#ifndef GL_SYNC_FLUSH_COMMANDS_BIT
	#define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
	#endif
#endif

#if CONFIG_GFX_OPENGL_ES == 1
// un-define ES 2.0 symbols in case headers are implicitly included,
// such as when including UIKit.h on iOS
	#ifdef GL_FRAMEBUFFER
	#undef GL_FRAMEBUFFER
	#endif
	#ifdef GL_FRAMEBUFFER_COMPLETE
	#undef GL_FRAMEBUFFER_COMPLETE
	#endif
	#ifdef GL_COLOR_ATTACHMENT0
	#undef GL_COLOR_ATTACHMENT0
	#endif
	#ifdef GL_FUNC_ADD
	#undef GL_FUNC_ADD
	#endif
	#ifdef GL_FUNC_SUBTRACT
	#undef GL_FUNC_SUBTRACT
	#endif
	#ifdef GL_FUNC_REVERSE_SUBTRACT
	#undef GL_FUNC_REVERSE_SUBTRACT
	#endif

#define glBlendEquation glBlendEquationOES
#define glGenFramebuffers glGenFramebuffersOES
#define glDeleteFramebuffers glDeleteFramebuffersOES
#define glCheckFramebufferStatus glCheckFramebufferStatusOES
#define glBindFramebuffer glBindFramebufferOES
#define glFramebufferTexture2D glFramebufferTexture2DOES
#define GL_FRAMEBUFFER GL_FRAMEBUFFER_OES
#define GL_FRAMEBUFFER_COMPLETE GL_FRAMEBUFFER_COMPLETE_OES
#define GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0_OES
#define GL_FUNC_ADD GL_FUNC_ADD_OES
#define GL_FUNC_SUBTRACT GL_FUNC_SUBTRACT_OES
#define GL_FUNC_REVERSE_SUBTRACT GL_FUNC_REVERSE_SUBTRACT_OES
#define GL_RGB5_A1 GL_RGB5_A1_OES
#define GL_RGB565 GL_RGB565_OES
#define GL_RGBA4 GL_RGBA4_OES

	#if defined __ANDROID__
	// glTexEnvi isn't supported in ES 1.0 but can be directly re-mapped
	#define glTexEnvi glTexEnvx
	#endif

	#ifndef GL_STREAM_DRAW
	#define GL_STREAM_DRAW GL_DYNAMIC_DRAW
	#endif

#define GL_CONSTANT_COLOR                 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR       0x8002
#define GL_CONSTANT_ALPHA                 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA       0x8004
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_LINK_STATUS                    0x8B82
#define GL_COMPILE_STATUS                 0x8B81
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C
typedef void (GL_APIENTRY  *GLDEBUGPROCKHR)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
#define GL_DEBUG_OUTPUT_KHR 0x92E0

// dummy ES 2.0 functions
namespace IG::Gfx
{
inline void glEnableVertexAttribArray(GLuint index) {}
inline void glReleaseShaderCompiler() {}
inline GLuint glCreateProgram(void) { return 0; }
inline GLuint glCreateShader(GLenum type) { return 0; }
inline void glDeleteProgram(GLuint program) {}
inline void glDeleteShader(GLuint shader) {}
inline void glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) {}
inline void glCompileShader(GLuint shader) {}
inline void glAttachShader(GLuint program, GLuint shader) {}
inline void glDetachShader(GLuint program, GLuint shader) {}
inline void glLinkProgram(GLuint program) {}
inline void glUseProgram(GLuint program) {}
inline GLint glGetUniformLocation(GLuint program, const GLchar *name) { return -1; }
inline void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name) {}
inline void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {}
inline void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) { *params = 0; }
inline void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {}
inline void glGetProgramiv(GLuint program, GLenum pname, GLint *params) { *params = 0; }
inline void glUniform1f(GLint location, GLfloat v0) {}
inline void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {}
inline void glUniform1i(GLint location, GLint v0) {}
inline void glUniform1iv(GLint location, GLsizei count, const GLint *value) {}
inline void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {}
inline void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {}
inline void glUniform2i(GLint location, GLint v0, GLint v1) {}
inline void glUniform2iv(GLint location, GLsizei count, const GLint *value) {}
inline void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {}
inline void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {}
inline void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {}
inline void glUniform3iv(GLint location, GLsizei count, const GLint *value) {}
inline void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {}
inline void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {}
inline void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {}
inline void glUniform4iv(GLint location, GLsizei count, const GLint *value) {}
inline void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {}
inline void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {}
inline void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {}
}
#endif

#ifndef GL_APIENTRY
#define GL_APIENTRY GLAPIENTRY
#endif
