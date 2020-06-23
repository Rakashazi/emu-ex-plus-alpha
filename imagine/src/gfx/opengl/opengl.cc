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
#include <imagine/gfx/Gfx.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/util/Interpolator.hh>
#include <imagine/util/string.h>
#include "private.hh"
#include "utils.h"

#ifndef GL_SYNC_GPU_COMMANDS_COMPLETE
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#endif

#ifndef GL_TIMEOUT_IGNORED
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFFull
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
static constexpr bool useGLCache = true;

void GLRenderer::verifyCurrentResourceContext()
{
	if(!Config::DEBUG_BUILD)
		return;
	auto currentCtx = Base::GLContext::current(glDpy);
	if(unlikely(gfxResourceContext != currentCtx))
	{
		bug_unreachable("expected GL context:%p but current is:%p", gfxResourceContext.nativeObject(), currentCtx.nativeObject());
	}
}

void GLRenderer::verifyCurrentTexture2D(TextureRef tex)
{
	if(!Config::DEBUG_BUILD)
		return;
	GLint realTexture = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &realTexture);
	if(tex != (GLuint)realTexture)
	{
		bug_unreachable("out of sync, expected %u but got %u, TEXTURE_2D", tex, realTexture);
	}
}

void Renderer::releaseShaderCompiler()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	runGLTask(
		[]()
		{
			glReleaseShaderCompiler();
		});
	#endif
}

void Renderer::autoReleaseShaderCompiler()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	releaseShaderCompilerEvent.notify();
	#endif
}

SyncFence Renderer::addResourceSyncFence()
{
	if(!resourceUpdate)
		return {};
	resourceUpdate = false;
	return addSyncFence();
}

SyncFence Renderer::addSyncFence()
{
	if(!useSeparateDrawContext)
		return {}; // no-op
	assumeExpr(support.hasSyncFences());
	GLsync sync;
	runGLTaskSync(
		[this, &sync]()
		{
			sync = support.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
			glFlush();
		});
	return sync;
}

void Renderer::deleteSyncFence(SyncFence fence)
{
	if(!fence.sync)
		return;
	assumeExpr(support.hasSyncFences());
	runGLTask(
		[this, sync = fence.sync]()
		{
			support.glDeleteSync(sync);
		});
}

void Renderer::clientWaitSync(SyncFence fence, uint64_t timeout)
{
	if(!fence.sync)
		return;
	assumeExpr(support.hasSyncFences());
	runGLTask(
		[this, sync = fence.sync, timeout]()
		{
			support.glClientWaitSync(sync, 0, timeout);
			support.glDeleteSync(sync);
		});
}

void Renderer::waitSync(SyncFence fence)
{
	if(!fence.sync)
		return;
	assumeExpr(support.hasSyncFences());
	runGLTask(
		[this, sync = fence.sync]()
		{
			support.glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
			support.glDeleteSync(sync);
		});
}

void Renderer::flush()
{
	runGLTask(
		[]()
		{
			glFlush();
		});
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
	runGLTaskSync(
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

void GLDrawableHolder::makeDrawable(Renderer &r, Base::Window &win)
{
	destroyDrawable(r);
	auto [ec, drawable] = r.glDpy.makeDrawable(win, r.gfxBufferConfig);
	if(ec)
	{
		logErr("Error creating GL drawable");
		return;
	}
	drawable_ = drawable;
	onResume =
		[drawable = drawable](bool focused) mutable
		{
			drawable.restoreCaches();
			return true;
		};
	Base::addOnResume(onResume, Base::RENDERER_DRAWABLE_ON_RESUME_PRIORITY);
	onExit =
		[this, glDpy = r.glDpy](bool backgrounded) mutable
		{
			if(backgrounded)
			{
				drawFinishedEvent.cancel();
				drawable_.freeCaches();
			}
			else
				glDpy.deleteDrawable(drawable_);
			return true;
		};
	Base::addOnExit(onExit, Base::RENDERER_DRAWABLE_ON_EXIT_PRIORITY);
	drawFinishedEvent.attach(
		[this]()
		{
			auto now = IG::steadyClockTimestamp();
			FrameParams frameParams{now, lastTimestamp, IG::FloatSeconds{0}};
			onFrame.runAll([&](Base::OnFrameDelegate del){ return del(frameParams); });
			lastTimestamp = now;
		});
}

void GLDrawableHolder::destroyDrawable(Renderer &r)
{
	if(!drawable_)
		return;
	r.glDpy.deleteDrawable(drawable_);
	drawable_ = {};
	Base::removeOnExit(onResume);
	Base::removeOnExit(onExit);
	drawFinishedEvent.detach();
	lastTimestamp = {};
}

bool DrawableHolder::addOnFrame(Base::OnFrameDelegate del, int priority)
{
	if(!onFrame.size())
	{
		// reset time-stamp when first delegate is added
		lastTimestamp = {};
	}
	return onFrame.add(del, priority);
}

bool DrawableHolder::removeOnFrame(Base::OnFrameDelegate del)
{
	return onFrame.remove(del);
}

void GLDrawableHolder::notifyOnFrame()
{
	if(onFrame.size())
	{
		drawFinishedEvent.notify();
	}
}

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
void GLRendererCommands::glcMatrixMode(GLenum mode)
{ if(useGLCache) glState.matrixMode(mode); else glMatrixMode(mode); }
#endif

void GLRendererCommands::glcBindTexture(GLenum target, GLuint texture)
{ if(useGLCache) glState.bindTexture(target, texture); else glBindTexture(target, texture); }
void GLRendererCommands::glcBlendFunc(GLenum sfactor, GLenum dfactor)
{ if(useGLCache) glState.blendFunc(sfactor, dfactor); else glBlendFunc(sfactor, dfactor); }
void GLRendererCommands::glcBlendEquation(GLenum mode)
{ if(useGLCache) glState.blendEquation(mode); else glBlendEquation(mode); }
void GLRendererCommands::glcEnable(GLenum cap)
{ if(useGLCache) glState.enable(cap); else glEnable(cap); }
void GLRendererCommands::glcDisable(GLenum cap)
{ if(useGLCache) glState.disable(cap); else glDisable(cap); }

GLboolean GLRendererCommands::glcIsEnabled(GLenum cap)
{
	if(useGLCache)
		return glState.isEnabled(cap);
	else
		return glIsEnabled(cap);
}

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
void GLRendererCommands::glcEnableClientState(GLenum cap)
{ if(useGLCache) glState.enableClientState(cap); else glEnableClientState(cap); }
void GLRendererCommands::glcDisableClientState(GLenum cap)
{ if(useGLCache) glState.disableClientState(cap); else glDisableClientState(cap); }
void GLRendererCommands::glcTexEnvi(GLenum target, GLenum pname, GLint param)
{ if(useGLCache) glState.texEnvi(target, pname, param); else glTexEnvi(target, pname, param); }
void GLRendererCommands::glcTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{ if(useGLCache) glState.texEnvfv(target, pname, params); else glTexEnvfv(target, pname, params); }
void GLRendererCommands::glcColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	if(useGLCache)
		glState.color4f(red, green, blue, alpha);
	else
	{
		glColor4f(red, green, blue, alpha);
		glState.colorState[0] = red; glState.colorState[1] = green; glState.colorState[2] = blue; glState.colorState[3] = alpha; // for color()
	}
}
#endif

bool GLRenderer::hasGLTask() const
{
	return mainTask && mainTask->isStarted();
}

void GLRenderer::runGLTask2(GLMainTask::FuncDelegate del, IG::Semaphore *semAddr)
{
	mainTask->runFunc(del, semAddr);
}

void GLRenderer::runGLTaskSync2(GLMainTask::FuncDelegate del)
{
	mainTask->runFuncSync(del);
}

void Renderer::waitAsyncCommands()
{
	runGLTaskSync([](){});
}

SyncFence::operator bool() const
{
	return sync;
}

}
