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
#include <imagine/gfx/DrawableHolder.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/base/Base.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Window.hh>
#include <imagine/thread/Thread.hh>
#include "internalDefs.hh"
#include "utils.hh"

namespace Gfx
{

GLRendererTask::GLRendererTask(Renderer &r):
	GLRendererTask{nullptr, r}
{}

GLRendererTask::GLRendererTask(const char *debugLabel, Renderer &r):
	GLTask{debugLabel}, r{&r}
{}

void GLRendererTask::initVBOs()
{
	#ifndef CONFIG_GFX_OPENGL_ES
	if(likely(streamVBO[0]))
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
	if(likely(streamVAO))
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
		Base::GLContext::setCurrent(Base::GLDisplay::getDefault(), glContext(), {});
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
	if(unlikely(!fbo))
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

void GLRendererTask::setDrawAsyncMode(DrawAsyncMode mode)
{
	autoDrawAsyncMode = mode;
}

Renderer &RendererTask::renderer() const
{
	return *r;
}

void GLRendererTask::doPreDraw(Base::Window &win, Base::WindowDrawParams winParams, DrawParams &params)
{
	if(unlikely(!context))
	{
		logWarn("draw() called without context");
		return;
	}
	auto &drawableHolder = winData(win).drawableHolder;
	if(unlikely(!drawableHolder))
	{
		drawableHolder.makeDrawable(r->glDpy, win, r->gfxBufferConfig);
	}
	if(params.asyncMode() == DrawAsyncMode::AUTO)
	{
		params.setAsyncMode(autoDrawAsyncMode);
	}
	if(unlikely(winParams.needsSync()))
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
	auto &drawableHolder = winData(win).drawableHolder;
	if(change.destroyed())
	{
		destroyDrawable(drawableHolder);
	}
	else if(!drawableHolder)
	{
		drawableHolder.makeDrawable(r->glDpy, win, r->gfxBufferConfig);
	}
	if(change.reset())
	{
		resetDrawable = true;
	}
}

void RendererTask::destroyDrawable(DrawableHolder &drawableHolder)
{
	if(!drawableHolder)
		return;
	run(
		[glContext = context, drawable = (Drawable)drawableHolder](TaskContext ctx)
		{
			// unset the drawable if it's currently in use
			if(glContext.hasCurrentDrawable(ctx.glDisplay(), drawable))
				glContext.setDrawable(ctx.glDisplay(), {}, glContext);
		}, true);
	drawableHolder.destroyDrawable();
}

bool GLRendererTask::handleDrawableReset()
{
	if(resetDrawable)
	{
		resetDrawable = false;
		return true;
	}
	return false;
}

void GLRendererTask::runInitialCommandsInGL(TaskContext ctx, DrawContextSupport &support)
{
	verifyCurrentContext(ctx.glDisplay());
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
}

void GLRendererTask::verifyCurrentContext(Base::GLDisplay glDpy) const
{
	if(!Config::DEBUG_BUILD)
		return;
	auto currentCtx = Base::GLContext::current(glDpy);
	if(unlikely(glContext() != currentCtx))
	{
		bug_unreachable("expected GL context:%p but current is:%p", glContext().nativeObject(), currentCtx.nativeObject());
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
		auto dpy = renderer().glDpy;
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
		auto dpy = renderer().glDpy;
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
	logMsg("set context:%p debug output:%s", glContext().nativeObject(), on ? "on" : "off");
	debugEnabled = on;
	run(
		[&support = renderer().support, on]()
		{
			support.setGLDebugOutput(on);
		});
}

RendererCommands GLRendererTask::makeRendererCommands(GLTask::TaskContext taskCtx, bool manageSemaphore,
	Base::Window &win, Viewport viewport, Mat4 projMat)
{
	initDefaultFramebuffer();
	auto &drawableHolder = winData(win).drawableHolder;
	RendererCommands cmds{*static_cast<RendererTask*>(this), &win, drawableHolder, taskCtx.glDisplay(),
		manageSemaphore ? taskCtx.semaphorePtr() : nullptr};
	cmds.setViewport(viewport);
	cmds.setProjectionMatrix(projMat);
	if(manageSemaphore)
		taskCtx.markSemaphoreNotified(); // semaphore will be notified in RendererCommands::present()
	return cmds;
}

}
