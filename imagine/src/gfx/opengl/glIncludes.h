#pragma once

#include <engine-globals.h>

// Automatic CONFIG_GFX_* settings

#if defined CONFIG_BASE_IOS || defined CONFIG_BASE_ANDROID || defined CONFIG_ENV_WEBOS
	#define CONFIG_GFX_OPENGL_ES 1
#endif

#if defined CONFIG_BASE_ANDROID
	#if CONFIG_ENV_ANDROID_MINSDK >= 9
		#define CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES 1
	#endif
	#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES) || defined(SUPPORT_ANDROID_DIRECT_TEXTURE)
		#define CONFIG_GFX_OPENGL_BUFFER_IMAGE_MULTI_IMPL 1
	#endif
#endif


// Header Locations

#if defined CONFIG_BASE_IOS
	#import <OpenGLES/ES1/gl.h>
	#import <OpenGLES/ES1/glext.h>
	#import <OpenGLES/ES2/gl.h>
#elif defined CONFIG_ENV_WEBOS // standard GL headers seem to be missing some extensions
	#define GL_GLEXT_PROTOTYPES
	#include <GLES/gl.h>
	#include <SDL/SDL_opengles_ext.h>
#elif defined CONFIG_BASE_PS3
	#define CONFIG_GFX_OPENGL_ES 1
	#include <PSGL/psgl.h>
#elif defined CONFIG_GFX_OPENGL_ES
	// Generic OpenGL ES headers
	#define GL_GLEXT_PROTOTYPES
	#include <GLES/gl.h>
	#include <GLES/glext.h>
#else
	// Generic OpenGL headers
	#define GL_GLEXT_PROTOTYPES
	#include <GL/gl.h>
	#include <GL/glext.h>
	#if defined CONFIG_BASE_X11
		#define CONFIG_GFX_OPENGL_GLX
		// TODO: fix when namespace support is finished
		#define Time X11Time_
		#define Pixmap X11Pixmap_
		#define GC X11GC_
		#define Window X11Window
		#define BOOL X11BOOL
		#define GLX_GLXEXT_PROTOTYPES
		#include <GL/glx.h>
		#include <GL/glxext.h>
		#undef Time
		#undef Pixmap
		#undef GC
		#undef Window
		#undef BOOL
	#endif
#endif

// Symbol Re-mapping

#ifdef CONFIG_GFX_OPENGL_ES
	#if !defined(CONFIG_BASE_PS3) && !defined(CONFIG_BASE_IOS)
		#define glBlendEquation glBlendEquationOES
		#define GL_FUNC_ADD GL_FUNC_ADD_OES
		#define GL_FUNC_SUBTRACT GL_FUNC_SUBTRACT_OES
		#define GL_FUNC_REVERSE_SUBTRACT GL_FUNC_REVERSE_SUBTRACT_OES
	#endif

	#ifdef GL_USE_OES_FIXED
		#define glTranslatef glTranslatex
		static void glTranslatex(TransformCoordinate x, TransformCoordinate y, TransformCoordinate z)
		{
			glTranslatexOES(TransformCoordinatePOD(x), TransformCoordinatePOD(y), TransformCoordinatePOD(z));
		}
		#define glScalef glScalex
		static void glScalex(TransformCoordinate sx, TransformCoordinate sy, TransformCoordinate sz)
		{
			glScalexOES(TransformCoordinatePOD(sx), TransformCoordinatePOD(sy), TransformCoordinatePOD(sz));
		}
		#define glRotatef glRotatex
		static void glRotatex(Angle angle, Angle x, Angle y, Angle z)
		{
			glRotatexOES(AnglePOD(angle), AnglePOD(x), AnglePOD(y), AnglePOD(z));
		}
	#endif
#endif

#if defined CONFIG_BASE_ANDROID
	// Android is missing GL_BGRA in ES 1 headers
	#define GL_BGRA 0x80E1
#elif defined CONFIG_GFX_OPENGL_ES && defined CONFIG_BASE_X11
	// Mesa uses GL_BGRA_EXT
	#ifndef GL_BGRA
		#define GL_BGRA GL_BGRA_EXT
	#endif
#endif
