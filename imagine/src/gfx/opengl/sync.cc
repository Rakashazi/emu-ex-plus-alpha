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

#define LOGTAG "GLSync"
#include <imagine/gfx/SyncFence.hh>
#include <imagine/gfx/opengl/GLRenderer.hh>
#include <imagine/logger/logger.h>
#include <cstring>

#ifndef EGL_SYNC_FENCE
#define EGL_SYNC_FENCE 0x30F9
#endif

#ifndef GL_SYNC_GPU_COMMANDS_COMPLETE
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#endif

namespace Gfx
{

void GLRenderer::setupFenceSync()
{
	#if !defined CONFIG_BASE_GL_PLATFORM_EGL && defined CONFIG_GFX_OPENGL_ES
	if(support.hasSyncFences())
		return;
	glManager.loadSymbol(support.glFenceSync, "glFenceSync");
	glManager.loadSymbol(support.glDeleteSync, "glDeleteSync");
	glManager.loadSymbol(support.glClientWaitSync, "glClientWaitSync");
	//glManager.loadSymbol(support.glWaitSync, "glWaitSync");
	#endif
}

void GLRenderer::setupAppleFenceSync()
{
	#if !defined CONFIG_BASE_GL_PLATFORM_EGL && defined CONFIG_GFX_OPENGL_ES
	if(support.hasSyncFences())
		return;
	glManager.loadSymbol(support.glFenceSync, "glFenceSyncAPPLE");
	glManager.loadSymbol(support.glDeleteSync, "glDeleteSyncAPPLE");
	glManager.loadSymbol(support.glClientWaitSync, "glClientWaitSyncAPPLE");
	//glManager.loadSymbol(support.glWaitSync, "glWaitSyncAPPLE");
	#endif
}

void GLRenderer::setupEglFenceSync(const char *eglExtenstionStr)
{
	if(Config::MACHINE_IS_PANDORA)	// TODO: driver waits for full timeout even if commands complete,
		return;												// possibly broken glFlush() behavior?
	if(support.hasSyncFences())
		return;
	#if defined CONFIG_BASE_GL_PLATFORM_EGL && defined CONFIG_GFX_OPENGL_ES
	// check for fence sync via EGL extensions
	if(strstr(eglExtenstionStr, "EGL_KHR_fence_sync"))
	{
		glManager.loadSymbol(support.eglCreateSync, "eglCreateSyncKHR");
		glManager.loadSymbol(support.eglDestroySync, "eglDestroySyncKHR");
		glManager.loadSymbol(support.eglClientWaitSync, "eglClientWaitSyncKHR");
		/*if(strstr(eglExtenstionStr, "EGL_KHR_wait_sync"))
		{
			glManager.loadSymbol(support.eglWaitSync, "eglWaitSyncKHR");
		}*/
	}
	#endif
}

bool DrawContextSupport::hasSyncFences() const
{
	#ifdef CONFIG_BASE_GL_PLATFORM_EGL
	if constexpr((bool)Config::Gfx::OPENGL_ES)
	{
		return eglCreateSync;
	}
	else
	{
		return true;
	}
	#else
	if constexpr((bool)Config::Gfx::OPENGL_ES)
	{
		return glFenceSync;
	}
	else
	{
		return Config::Gfx::OPENGL_SHADER_PIPELINE;
	}
	#endif
}

bool DrawContextSupport::hasServerWaitSync() const
{
	return false;
	/*#ifdef CONFIG_BASE_GL_PLATFORM_EGL
	if constexpr(Config::Gfx::OPENGL_ES)
	{
		return eglWaitSync;
	}
	else
	{
		return true;
	}
	#else
	if constexpr(Config::Gfx::OPENGL_ES)
	{
		return glWaitSync;
	}
	else
	{
		return Config::Gfx::OPENGL_SHADER_PIPELINE;
	}
	#endif*/
}

GLsync DrawContextSupport::fenceSync(Base::GLDisplay dpy)
{
	#ifdef CONFIG_BASE_GL_PLATFORM_EGL
	return static_cast<GLsync>(eglCreateSync(dpy, EGL_SYNC_FENCE, nullptr));
	#else
	return glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	#endif
}

void DrawContextSupport::deleteSync(Base::GLDisplay dpy, GLsync sync)
{
	#ifdef CONFIG_BASE_GL_PLATFORM_EGL
	if(bool success = eglDestroySync(dpy, sync);
		Config::DEBUG_BUILD && !success)
	{
		logErr("error:%s in eglDestroySync(%p, %p)", Base::GLManager::errorString(eglGetError()), (EGLDisplay)dpy, sync);
	}
	#else
	glDeleteSync(sync);
	#endif
}

GLenum DrawContextSupport::clientWaitSync(Base::GLDisplay dpy, GLsync sync, GLbitfield flags, GLuint64 timeout)
{
	#ifdef CONFIG_BASE_GL_PLATFORM_EGL
	if(auto status = eglClientWaitSync(dpy, sync, flags, timeout);
		Config::DEBUG_BUILD && !status)
	{
		logErr("error:%s in eglClientWaitSync(%p, %p, 0x%X, %llu)",
			Base::GLManager::errorString(eglGetError()), (EGLDisplay)dpy, sync, flags, (unsigned long long)timeout);
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

void DrawContextSupport::waitSync(Base::GLDisplay dpy, GLsync sync)
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

SyncFence::operator bool() const
{
	return sync;
}

}
