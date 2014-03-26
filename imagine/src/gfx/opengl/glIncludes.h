#pragma once

#include <engine-globals.h>
#include <config/machine.hh>

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
	#if CONFIG_ENV_ANDROID_MINSDK >= 9
	#define CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES 1
	#endif
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
	#if defined CONFIG_BASE_X11
	#define CONFIG_GFX_OPENGL_GLX
	#include <base/x11/glxIncludes.h>
	#endif
#endif

// Symbol Re-mapping

#if defined CONFIG_GFX_OPENGL_ES && CONFIG_GFX_OPENGL_ES_MAJOR_VERSION == 1
#define glBlendEquation glBlendEquationOES

	#if defined __ANDROID__
	// glTexEnvi isn't supported in ES 1.0 but can be directly re-mapped
	#define glTexEnvi glTexEnvx
	#endif

	#ifndef GL_FUNC_ADD
	#define GL_FUNC_ADD GL_FUNC_ADD_OES
	#endif
	#ifndef GL_FUNC_SUBTRACT
	#define GL_FUNC_SUBTRACT GL_FUNC_SUBTRACT_OES
	#endif
	#ifndef GL_FUNC_REVERSE_SUBTRACT
	#define GL_FUNC_REVERSE_SUBTRACT GL_FUNC_REVERSE_SUBTRACT_OES
	#endif
	#ifndef GL_STREAM_DRAW
	#define GL_STREAM_DRAW GL_DYNAMIC_DRAW
	#endif
#endif

#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif
