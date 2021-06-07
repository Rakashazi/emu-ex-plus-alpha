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

#define LOGTAG "RendererTask"
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Window.hh>
#include <imagine/thread/Thread.hh>
#include "internalDefs.hh"
#include "utils.hh"

namespace Gfx
{

GLRendererTask::GLRendererTask(Base::ApplicationContext ctx, Renderer &r):
	GLRendererTask{ctx, nullptr, r}
{}

GLRendererTask::GLRendererTask(Base::ApplicationContext ctx, const char *debugLabel, Renderer &r):
	GLTask{ctx, debugLabel}, r{&r}
{}

void GLRendererTask::initVBOs()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(streamVBO[0]) [[likely]]
		return;
	logMsg("making stream VBO");
	glGenBuffers(streamVBO.size(), streamVBO.data());
	#endif
}

GLuint GLRendererTask::getVBO()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	assert(streamVBO[streamVBOIdx]);
	auto vbo = streamVBO[streamVBOIdx];
	streamVBOIdx = (streamVBOIdx+1) % streamVBO.size();
	return vbo;
	#else
	return 0;
	#endif
}

void GLRendererTask::initVAO()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(streamVAO) [[likely]]
		return;
	logMsg("making stream VAO");
	glGenVertexArrays(1, &streamVAO);
	glBindVertexArray(streamVAO);
	#endif
}

void GLRendererTask::initDefaultFramebuffer()
{
	if(Config::Gfx::GLDRAWABLE_NEEDS_FRAMEBUFFER && !defaultFB)
	{
		glContext().setCurrentContext({});
		GLuint fb;
		glGenFramebuffers(1, &fb);
		logMsg("created default framebuffer:%u", fb);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		defaultFB = fb;
	}
}

GLuint GLRendererTask::bindFramebuffer(Texture &tex)
{
	assert(tex);
	if(!fbo) [[unlikely]]
	{
		glGenFramebuffers(1, &fbo);
		logMsg("init FBO:0x%X", fbo);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.texName(), 0);
	return fbo;
}

void GLRendererTask::setRenderer(Renderer *r_)
{
	r = r_;
}

Renderer &RendererTask::renderer() const
{
	return *r;
}

void GLRendererTask::doPreDraw(Base::Window &win, Base::WindowDrawParams winParams, DrawParams &params) const
{
	if(!context) [[unlikely]]
	{
		logWarn("draw() called without context");
		return;
	}
	assumeExpr(winData(win).drawable);
	if(params.asyncMode() == DrawAsyncMode::AUTO)
	{
		params.setAsyncMode(DrawAsyncMode::PRESENT);
	}
	if(winParams.needsSync()) [[unlikely]]
	{
		params.setAsyncMode(DrawAsyncMode::NONE);
	}
}

RendererTask::operator bool() const
{
	return GLTask::operator bool();
}

void RendererTask::updateDrawableForSurfaceChange(Base::Window &win, Base::WindowSurfaceChange change)
{
	auto &drawable = winData(win).drawable;
	switch(change.action())
	{
		case Base::WindowSurfaceChange::Action::CREATED:
			r->makeWindowDrawable(*this, win, winData(win).bufferConfig, winData(win).colorSpace);
			return;
		case Base::WindowSurfaceChange::Action::CHANGED:
			if(change.surfaceResized())
			{
				run(
					[this, drawable = (Drawable)drawable](TaskContext ctx)
					{
						// reset the drawable if it's currently in use
						if(Base::GLManager::hasCurrentDrawable(drawable))
							context.setCurrentDrawable(drawable);
					});
			}
			return;
		case Base::WindowSurfaceChange::Action::DESTROYED:
			destroyDrawable(drawable);
			return;
	}
}

void GLRendererTask::destroyDrawable(Base::GLDrawable &drawable)
{
	if(!drawable)
		return;
	run(
		[this, drawable = (Drawable)drawable](TaskContext ctx)
		{
			// unset the drawable if it's currently in use
			if(Base::GLManager::hasCurrentDrawable(drawable))
				context.setCurrentDrawable({});
		}, true);
	drawable = {};
}

void GLRendererTask::runInitialCommandsInGL(TaskContext ctx, DrawContextSupport &support)
{
	verifyCurrentContext();
	if(support.hasDebugOutput && defaultToFullErrorChecks)
	{
		debugEnabled = true;
		support.setGLDebugOutput(true);
	}
	if(support.hasVBOFuncs)
		initVBOs();
	#ifndef CONFIG_GFX_OPENGL_ES
	if(!support.useFixedFunctionPipeline)
		initVAO();
	#endif
	ctx.notifySemaphore();
	runGLCheckedVerbose([&]()
	{
		glEnableVertexAttribArray(VATTR_POS);
	}, "glEnableVertexAttribArray(VATTR_POS)");
	glClearColor(0., 0., 0., 1.);
	if constexpr((bool)Config::Gfx::OPENGL_ES)
	{
		if(support.hasSrgbWriteControl)
		{
			glDisable(GL_FRAMEBUFFER_SRGB);
		}
	}
}

void GLRendererTask::verifyCurrentContext() const
{
	if(!Config::DEBUG_BUILD)
		return;
	auto currentCtx = Base::GLManager::currentContext();
	if(currentCtx != glContext()) [[unlikely]]
	{
		bug_unreachable("expected GL context:%p but current is:%p", (Base::NativeGLContext)glContext(), currentCtx);
	}
}

SyncFence RendererTask::addSyncFence()
{
	if(!r->support.hasSyncFences())
		return {}; // no-op
	GLsync sync;
	runSync(
		[&support = r->support, &sync](TaskContext ctx)
		{
			sync = support.fenceSync(ctx.glDisplay());
		});
	return sync;
}

void RendererTask::deleteSyncFence(SyncFence fence)
{
	if(!fence.sync)
		return;
	assumeExpr(r->support.hasSyncFences());
	const bool canPerformInCurrentThread = Config::Base::GL_PLATFORM_EGL;
	if(canPerformInCurrentThread)
	{
		auto dpy = renderer().glDisplay();
		renderer().support.deleteSync(dpy, fence.sync);
	}
	else
	{
		run(
			[&support = r->support, sync = fence.sync](TaskContext ctx)
			{
				support.deleteSync(ctx.glDisplay(), sync);
			});
	}
}

void RendererTask::clientWaitSync(SyncFence fence, int flags, std::chrono::nanoseconds timeout)
{
	if(!fence.sync)
		return;
	assumeExpr(r->support.hasSyncFences());
	const bool canPerformInCurrentThread = Config::Base::GL_PLATFORM_EGL && !flags;
	if(canPerformInCurrentThread)
	{
		//logDMsg("waiting on sync:%p flush:%s timeout:0%llX", fence.sync, flags & 1 ? "yes" : "no", (unsigned long long)timeout);
		auto dpy = renderer().glDisplay();
		renderer().support.clientWaitSync(dpy, fence.sync, 0, timeout.count());
		renderer().support.deleteSync(dpy, fence.sync);
	}
	else
	{
		runSync(
			[&support = r->support, sync = fence.sync, timeout, flags](TaskContext ctx)
			{
				support.clientWaitSync(ctx.glDisplay(), sync, flags, timeout.count());
				ctx.notifySemaphore();
				support.deleteSync(ctx.glDisplay(), sync);
			});
	}
}

SyncFence RendererTask::clientWaitSyncReset(SyncFence fence, int flags, std::chrono::nanoseconds timeout)
{
	clientWaitSync(fence, flags, timeout);
	return addSyncFence();
}

void RendererTask::waitSync(SyncFence fence)
{
	if(!fence.sync)
		return;
	assumeExpr(r->support.hasSyncFences());
	run(
		[&support = r->support, sync = fence.sync](TaskContext ctx)
		{
			support.waitSync(ctx.glDisplay(), sync);
			support.deleteSync(ctx.glDisplay(), sync);
		});
}

void RendererTask::awaitPending()
{
	if(!*this)
		return;
	runSync([](){});
}

void RendererTask::flush()
{
	run(
		[]()
		{
			glFlush();
		});
}

void RendererTask::releaseShaderCompiler()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	run(
		[]()
		{
			glReleaseShaderCompiler();
		});
	#endif
}

void RendererTask::setDebugOutput(bool on)
{
	if(!renderer().support.hasDebugOutput || debugEnabled == on)
	{
		return;
	}
	logMsg("set context:%p debug output:%s", (Base::NativeGLContext)glContext(), on ? "on" : "off");
	debugEnabled = on;
	run(
		[&support = renderer().support, on]()
		{
			support.setGLDebugOutput(on);
		});
}

RendererCommands GLRendererTask::makeRendererCommands(GLTask::TaskContext taskCtx, bool manageSemaphore,
	bool notifyWindowAfterPresent, Base::Window &win, Viewport viewport, Mat4 projMat)
{
	initDefaultFramebuffer();
	auto &drawable = winData(win).drawable;
	RendererCommands cmds{*static_cast<RendererTask*>(this),
		notifyWindowAfterPresent ? &win : nullptr, drawable, taskCtx.glDisplay(),
		glContext(), manageSemaphore ? taskCtx.semaphorePtr() : nullptr};
	cmds.setViewport(viewport);
	cmds.setProjectionMatrix(projMat);
	if(manageSemaphore)
		taskCtx.markSemaphoreNotified(); // semaphore will be notified in RendererCommands::present()
	return cmds;
}

}
