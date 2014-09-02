#pragma once

#include <imagine/engine-globals.h>

// Automatic CONFIG_GFX_* settings

#ifndef CONFIG_GFX_OPENGL_ES
	#if defined CONFIG_BASE_IOS || defined __ANDROID__ || defined CONFIG_ENV_WEBOS || defined CONFIG_MACHINE_PANDORA
	#define CONFIG_GFX_OPENGL_ES 1
	#endif
#endif

#ifdef CONFIG_GFX_OPENGL_ES
	#ifndef CONFIG_GFX_OPENGL_ES_MAJOR_VERSION
	#error "CONFIG_GFX_OPENGL_ES_MAJOR_VERSION isn't defined"
	#endif
#endif

#if defined __ANDROID__
	#define CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES 1
	#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES) || defined(SUPPORT_ANDROID_DIRECT_TEXTURE)
	#define CONFIG_GFX_OPENGL_BUFFER_IMAGE_MULTI_IMPL 1
	#endif
#endif

#if !defined CONFIG_BASE_MACOSX && \
	((defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1) || !defined CONFIG_GFX_OPENGL_ES)
#define CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
#endif
#if (defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 2) || !defined CONFIG_GFX_OPENGL_ES
#define CONFIG_GFX_OPENGL_SHADER_PIPELINE
#endif

// Header Locations For Platform

#if defined CONFIG_BASE_IOS
	#if CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1
	#import <OpenGLES/ES1/gl.h>
	#import <OpenGLES/ES1/glext.h>
	using GLchar = char;
	#else
	#import <OpenGLES/ES2/gl.h>
	#import <OpenGLES/ES2/glext.h>
	#endif
#elif defined CONFIG_BASE_MACOSX
#import <OpenGL/gl3.h>
#import <OpenGL/gl3ext.h>
#elif defined CONFIG_ENV_WEBOS // standard GL headers seem to be missing some extensions
#define GL_GLEXT_PROTOTYPES
#include <GLES/gl.h>
#include <SDL/SDL_opengles_ext.h>
#elif defined CONFIG_BASE_WIN32
#include <util/windows/windows.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/wglew.h>
#elif defined CONFIG_GFX_OPENGL_ES // Generic OpenGL ES headers
#define GL_GLEXT_PROTOTYPES
	#if CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1
	#include <GLES/gl.h>
	#include <GLES/glext.h>
		#ifdef CONFIG_MACHINE_PANDORA
		using GLchar = char;
		#endif
	#else
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
		#ifdef CONFIG_MACHINE_PANDORA
		#include <GLES2/gl2extimg.h> // missing extensions
		using GLchar = char;
		#endif
	#endif
#else // Generic OpenGL headers
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

// Symbol Re-mapping

#if defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1
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

	#if defined __ANDROID__
	// glTexEnvi isn't supported in ES 1.0 but can be directly re-mapped
	#define glTexEnvi glTexEnvx
	#endif

	#ifndef GL_STREAM_DRAW
	#define GL_STREAM_DRAW GL_DYNAMIC_DRAW
	#endif
#endif

#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif
