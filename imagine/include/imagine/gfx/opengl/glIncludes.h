#pragma once

#include <imagine/config/defs.hh>

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
#endif

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef GL_APIENTRY
#define GL_APIENTRY APIENTRY
#endif

#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif

#ifndef GL_MIRRORED_REPEAT
#define GL_MIRRORED_REPEAT 0x8370
#endif

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif

#ifndef GL_RGB8
#define GL_RGB8 0x8051
#endif

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
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

#ifndef GL_LUMINANCE
#define GL_LUMINANCE 0x1909
#endif

#ifndef GL_LUMINANCE_ALPHA
#define GL_LUMINANCE_ALPHA 0x190A
#endif

#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif

#ifndef GL_BGR
#define GL_BGR 0x80E0
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

#ifndef GL_R8
#define GL_R8 0x8229
#endif

#ifndef GL_RG
#define GL_RG 0x8227
#endif

#ifndef GL_RG8
#define GL_RG8 0x822B
#endif

#ifndef GL_PIXEL_PACK_BUFFER
#define GL_PIXEL_PACK_BUFFER 0x88EB
#endif

#ifndef GL_PIXEL_UNPACK_BUFFER
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif

#ifndef GL_APICALL
#define GL_APICALL
#endif
