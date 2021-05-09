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
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/string.h>
#include <imagine/logger/logger.h>

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

void DrawContextSupport::setGLDebugOutput(bool on)
{
	#ifdef CONFIG_GFX_OPENGL_DEBUG_CONTEXT
	if(!hasDebugOutput)
		return;
	if(!on)
	{
		glDisable(DEBUG_OUTPUT);
	}
	else
	{
		if(!glDebugMessageCallback) [[unlikely]]
		{
			auto glDebugMessageCallbackStr =
					Config::Gfx::OPENGL_ES ? "glDebugMessageCallbackKHR" : "glDebugMessageCallback";
			logWarn("enabling debug output with %s", glDebugMessageCallbackStr);
			glDebugMessageCallback = (typeof(glDebugMessageCallback))Base::GLManager::procAddress(glDebugMessageCallbackStr);
		}
		glDebugMessageCallback(
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
		glEnable(DEBUG_OUTPUT);
	}
	#endif
}

}
