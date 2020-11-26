/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "GLRenderer"
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/DrawableHolder.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/util/Interpolator.hh>
#include <imagine/util/string.h>
#include "private.hh"
#include "utils.h"
#ifdef __ANDROID__
#include <imagine/base/platformExtras.hh>
#endif

#ifndef GL_DEBUG_TYPE_ERROR
#define GL_DEBUG_TYPE_ERROR 0x824C
#endif

#ifndef GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#endif

#ifndef GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x824E
#endif

#ifndef GL_DEBUG_TYPE_PORTABILITY
#define GL_DEBUG_TYPE_PORTABILITY 0x824F
#endif

#ifndef GL_DEBUG_TYPE_PERFORMANCE
#define GL_DEBUG_TYPE_PERFORMANCE 0x8250
#endif

#ifndef GL_DEBUG_TYPE_OTHER
#define GL_DEBUG_TYPE_OTHER 0x8251
#endif

#ifndef GL_DEBUG_SEVERITY_HIGH
#define GL_DEBUG_SEVERITY_HIGH 0x9146
#endif

#ifndef GL_DEBUG_SEVERITY_MEDIUM
#define GL_DEBUG_SEVERITY_MEDIUM 0x9147
#endif

#ifndef GL_DEBUG_SEVERITY_LOW
#define GL_DEBUG_SEVERITY_LOW 0x9148
#endif

namespace Gfx
{

bool checkGLErrors = Config::DEBUG_BUILD;
bool checkGLErrorsVerbose = false;

void Renderer::releaseShaderCompiler()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	task().releaseShaderCompiler();
	#endif
}

void Renderer::autoReleaseShaderCompiler()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	releaseShaderCompilerEvent.notify();
	#endif
}

void Renderer::setCorrectnessChecks(bool on)
{
	if(on)
	{
		logWarn("enabling verification of OpenGL state");
	}
	GLStateCache::verifyState = on;
	checkGLErrors = on ? true : Config::DEBUG_BUILD;
	checkGLErrorsVerbose = on;
}

static const char *debugTypeToStr(GLenum type)
{
	switch(type)
	{
		case GL_DEBUG_TYPE_ERROR: return "Error";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated Behavior";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "Undefined Behavior";
		case GL_DEBUG_TYPE_PORTABILITY: return "Portability";
		case GL_DEBUG_TYPE_PERFORMANCE: return "Performance";
		default: [[fallthrough]];
		case GL_DEBUG_TYPE_OTHER: return "Other";
	}
}

static LoggerSeverity severityToLogger(GLenum severity)
{
	switch(severity)
	{
		default: [[fallthrough]];
		case GL_DEBUG_SEVERITY_LOW: return LOGGER_DEBUG_MESSAGE;
		case GL_DEBUG_SEVERITY_MEDIUM: return LOGGER_WARNING;
		case GL_DEBUG_SEVERITY_HIGH: return LOGGER_ERROR;
	}
}

void setGLDebugOutput(DrawContextSupport &support, bool on)
{
	assumeExpr(support.hasDebugOutput);
	#ifdef CONFIG_GFX_OPENGL_DEBUG_CONTEXT
	if(!on)
	{
		glDisable(support.DEBUG_OUTPUT);
	}
	else
	{
		if(unlikely(!support.glDebugMessageCallback))
		{
			auto glDebugMessageCallbackStr =
					Config::Gfx::OPENGL_ES ? "glDebugMessageCallbackKHR" : "glDebugMessageCallback";
			logWarn("enabling debug output with %s", glDebugMessageCallbackStr);
			support.glDebugMessageCallback = (typeof(support.glDebugMessageCallback))Base::GLContext::procAddress(glDebugMessageCallbackStr);
		}
		support.glDebugMessageCallback(
			GL_APIENTRY [](GLenum source, GLenum type, GLuint id,
				GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
			{
				if(Config::envIsAndroid && string_equal(message, "FreeAllocationOnTimestamp - WaitForTimestamp"))
				{
					return;
				}
				if(Config::envIsLinux && type == GL_DEBUG_TYPE_OTHER)
				{
					return;
				}
				logger_modulePrintfn(severityToLogger(severity), "%s: %s", debugTypeToStr(type), message);
			}, nullptr);
		glEnable(support.DEBUG_OUTPUT);
	}
	#endif
}

void Renderer::setDebugOutput(bool on)
{
	if(!Config::DEBUG_BUILD || !support.hasDebugOutput)
	{
		return;
	}
	task().runSync(
		[this, on]()
		{
			setGLDebugOutput(support, on);
		});
}

ClipRect Renderer::makeClipRect(const Base::Window &win, IG::WindowRect rect)
{
	int x = rect.x;
	int y = rect.y;
	int w = rect.xSize();
	int h = rect.ySize();
	//logMsg("scissor before transform %d,%d size %d,%d", x, y, w, h);
	// translate from view to window coordinates
	if(!Config::SYSTEM_ROTATES_WINDOWS)
	{
		using namespace Base;
		switch(win.softOrientation())
		{
			bcase VIEW_ROTATE_0:
				//x += win.viewport.rect.x;
				y = win.height() - (y + h);
			bcase VIEW_ROTATE_90:
				//x += win.viewport.rect.y;
				//y = win.width() - (y + h /*+ (win.w - win.viewport.rect.x2)*/);
				std::swap(x, y);
				std::swap(w, h);
				x = (win.realWidth() - x) - w;
				y = (win.realHeight() - y) - h;
			bcase VIEW_ROTATE_270:
				//x += win.viewport.rect.y;
				//y += win.viewport.rect.x;
				std::swap(x, y);
				std::swap(w, h);
			bcase VIEW_ROTATE_180:
				x = (win.realWidth() - x) - w;
				//y = win.height() - (y + h);
				//std::swap(x, y);
				//std::swap(w, h);
				//x += win.viewport.rect.x;
				//y += win.height() - win.viewport.bounds().y2;
		}
	}
	else
	{
		//x += win.viewport.rect.x;
		y = win.height() - (y + h /*+ win.viewport.rect.y*/);
	}
	return {x, y, w, h};
}

bool Renderer::supportsSyncFences() const
{
	return support.hasSyncFences();
}

void Renderer::setPresentationTime(Drawable drawable, IG::FrameTime time) const
{
	#ifdef __ANDROID__
	if(!support.eglPresentationTimeANDROID)
		return;
	bool success = support.eglPresentationTimeANDROID(glDpy, drawable, time.count());
	if(Config::DEBUG_BUILD && !success)
	{
		logErr("error:%s in eglPresentationTimeANDROID(%p, %llu)",
			glDpy.errorString(eglGetError()), (EGLSurface)drawable, (unsigned long long)time.count());
	}
	#endif
}

unsigned Renderer::maxSwapChainImages() const
{
	#ifdef __ANDROID__
	if(Base::androidSDK() < 18)
		return 2;
	#endif
	return 3; // assume triple-buffering by default
}

}
