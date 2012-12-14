#pragma once

#include <engine-globals.h>

#if defined(CONFIG_BASE_IOS)
	#define CONFIG_GFX_OPENGL_ES 1
	#import <OpenGLES/ES1/gl.h>
	#import <OpenGLES/ES1/glext.h>
	#import <OpenGLES/ES2/gl.h>
#elif defined(CONFIG_BASE_ANDROID)
	#define CONFIG_GFX_OPENGL_ES 1
	#define GL_GLEXT_PROTOTYPES
	#include <GLES/gl.h>
	#include <GLES/glext.h>
	#if CONFIG_ENV_ANDROID_MINSDK >= 9
		#define CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES 1
	#endif
	#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES) || defined(SUPPORT_ANDROID_DIRECT_TEXTURE)
		#define CONFIG_GFX_OPENGL_BUFFER_IMAGE_MULTI_IMPL 1
	#endif
#elif defined(CONFIG_ENV_WEBOS) // standard GL headers seem to be missing some extensions
	#define CONFIG_GFX_OPENGL_ES 1
	#define GL_GLEXT_PROTOTYPES
	#include <GLES/gl.h>
	#include <SDL/SDL_opengles_ext.h>
#elif defined(CONFIG_BASE_PS3)
	#define CONFIG_GFX_OPENGL_ES 1
	#include <PSGL/psgl.h>
#elif defined(CONFIG_BASE_X11)
	#if defined(CONFIG_GFX_OPENGL_ES)
		#define GL_GLEXT_PROTOTYPES
		#include <GLES/gl.h>
		#include <GLES/glext.h>
	#else
		#define CONFIG_GFX_OPENGL_GLX
	#endif
#endif

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

#ifdef CONFIG_GFX_OPENGL_GLEW_STATIC
	#define GLEW_STATIC
	#include <util/glew/glew.h>

	#ifdef CONFIG_BASE_WIN32
		#define USE_WGL_ARB_extensions_string
		#define USE_WGL_EXT_extensions_string
		#define USE_WGL_ARB_pixel_format
		#define USE_WGL_EXT_swap_control
		#define USE_WGL_ARB_multisample
		#include <util/glew/wglew.h>
	#endif
#elif !defined(CONFIG_GFX_OPENGL_ES)
	#ifdef CONFIG_BASE_X11
		#define GL_GLEXT_PROTOTYPES
		#include <GL/gl.h>
		#include <GL/glext.h>
		#if defined CONFIG_BASE_X11
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
	#elif defined CONFIG_BASE_MACOSX
		#define GL_GLEXT_PROTOTYPES
		#include <GL/gl.h>
		#include <GL/glext.h>
	#else
		// for using the standard GLEW lib
		#include <GL/glew.h>
		#ifdef CONFIG_BASE_WIN32
			#include <GL/wglew.h>
		#endif
	#endif
#endif
