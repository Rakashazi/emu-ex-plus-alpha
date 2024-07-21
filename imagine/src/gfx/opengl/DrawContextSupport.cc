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

#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>
#include "internalDefs.hh"

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

#ifndef EGL_SYNC_FENCE
#define EGL_SYNC_FENCE 0x30F9
#endif

#ifndef GL_SYNC_GPU_COMMANDS_COMPLETE
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#endif

namespace IG::Gfx
{

constexpr SystemLogger log{"GLRenderer"};

bool DrawContextSupport::hasSyncFences() const
{
	if constexpr((bool)Config::Gfx::OPENGL_ES)
	{
		#ifdef CONFIG_BASE_GL_PLATFORM_EGL
		return (bool)eglCreateSync;
		#else
		return glFenceSync;
		#endif
	}
	else
	{
		return true;
	}
}

bool DrawContextSupport::hasServerWaitSync() const
{
	return false;
	/*
	if constexpr(Config::Gfx::OPENGL_ES)
	{
		#ifdef CONFIG_BASE_GL_PLATFORM_EGL
		return eglWaitSync;
		#else
		return glWaitSync;
		#endif
	}
	else
	{
		return true;
	}
	*/
}

GLsync DrawContextSupport::fenceSync([[maybe_unused]] GLDisplay dpy)
{
	#ifdef CONFIG_BASE_GL_PLATFORM_EGL
	return static_cast<GLsync>(eglCreateSync(dpy, EGL_SYNC_FENCE, nullptr));
	#else
	return glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	#endif
}

void DrawContextSupport::deleteSync([[maybe_unused]] GLDisplay dpy, GLsync sync)
{
	#ifdef CONFIG_BASE_GL_PLATFORM_EGL
	if(bool success = eglDestroySync(dpy, sync);
		Config::DEBUG_BUILD && !success)
	{
		log.error("error:{} in eglDestroySync({}, {})", GLManager::errorString(eglGetError()), (EGLDisplay)dpy, (void*)sync);
	}
	#else
	glDeleteSync(sync);
	#endif
}

GLenum DrawContextSupport::clientWaitSync([[maybe_unused]] GLDisplay dpy, GLsync sync, GLbitfield flags, GLuint64 timeout)
{
	#ifdef CONFIG_BASE_GL_PLATFORM_EGL
	if(auto status = eglClientWaitSync(dpy, sync, flags, timeout);
		Config::DEBUG_BUILD && !status)
	{
		log.error("error:{} in eglClientWaitSync({}, {}, {:X}, {})",
			GLManager::errorString(eglGetError()), (EGLDisplay)dpy, (void*)sync, flags, timeout);
		return status;
	}
	else
	{
		return status;
	}
	#else
	return glClientWaitSync(sync, flags, timeout);
	#endif
}

void DrawContextSupport::waitSync(GLDisplay, GLsync)
{
	bug_unreachable("waitSync() not currently used");
	/*#ifdef CONFIG_BASE_GL_PLATFORM_EGL
	if constexpr(Config::Gfx::OPENGL_ES)
	{
		if(!eglWaitSync)
		{
			eglClientWaitSync(dpy, sync, 0, SyncFence::IGNORE_TIMEOUT);
			return;
		}
	}
	eglWaitSync(dpy, sync, 0);
	#else
	glWaitSync(sync, 0, SyncFence::IGNORE_TIMEOUT);
	#endif*/
}

bool DrawContextSupport::hasDrawReadBuffers() const
{
	#ifdef CONFIG_GFX_OPENGL_ES
	return false; //glDrawBuffers;
	#else
	return true;
	#endif
}

#ifdef __ANDROID__
bool DrawContextSupport::hasEGLTextureStorage() const
{
	return glEGLImageTargetTexStorageEXT;
}
#endif

bool DrawContextSupport::hasImmutableBufferStorage() const
{
	#ifdef CONFIG_GFX_OPENGL_ES
	return glBufferStorage;
	#else
	return hasBufferStorage;
	#endif
}

bool DrawContextSupport::hasMemoryBarriers() const
{
	return false;
	/*#ifdef CONFIG_GFX_OPENGL_ES
	return glMemoryBarrier;
	#else
	return hasMemoryBarrier;
	#endif*/
}

bool DrawContextSupport::hasVAOFuncs() const
{
	if constexpr(Config::DEBUG_BUILD && forceNoVAOs)
		return false;
	#ifdef CONFIG_GFX_OPENGL_ES
	return glBindVertexArray;
	#else
	return true;
	#endif
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
	if(!hasDebugOutput)
		return;
	if(!on)
	{
		glDisable(GL_DEBUG_OUTPUT);
	}
	else
	{
		if(!glDebugMessageCallback) [[unlikely]]
		{
			log.warn("enabling debug output with {}", glDebugMessageCallbackName);
			glDebugMessageCallback = (typeof(glDebugMessageCallback))GLManager::procAddress(glDebugMessageCallbackName);
		}
		glDebugMessageCallback(
			GL_APIENTRY []([[maybe_unused]] GLenum source, GLenum type, [[maybe_unused]] GLuint id,
				GLenum severity, [[maybe_unused]] GLsizei length, const GLchar* message, [[maybe_unused]] const void* userParam)
			{
				std::string_view msgString{message, size_t(length)};
				if(Config::envIsAndroid && (msgString == "FreeAllocationOnTimestamp - WaitForTimestamp"
					|| msgString.contains("Submission has been flushed")))
				{
					return;
				}
				if(Config::envIsLinux && type == GL_DEBUG_TYPE_OTHER)
				{
					return;
				}
				logger_modulePrintfn(severityToLogger(severity), "%s: %s", debugTypeToStr(type), message);
				if(severity == GL_DEBUG_SEVERITY_HIGH && type != GL_DEBUG_TYPE_PERFORMANCE)
					abort();
			}, nullptr);
		glEnable(GL_DEBUG_OUTPUT);
	}
}

}
