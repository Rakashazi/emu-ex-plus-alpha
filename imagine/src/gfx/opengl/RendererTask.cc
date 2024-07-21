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

#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Window.hh>
#include <imagine/thread/Thread.hh>
#include "internalDefs.hh"
#include "utils.hh"

namespace IG::Gfx
{

constexpr SystemLogger log{"RendererTask"};

GLRendererTask::GLRendererTask(ApplicationContext ctx, Renderer &r):
	GLRendererTask{ctx, nullptr, r} {}

GLRendererTask::GLRendererTask(ApplicationContext ctx, const char *debugLabel, Renderer &r):
	GLTask{ctx, debugLabel}, r{&r} {}

void GLRendererTask::initDefaultFramebuffer()
{
	if(Config::Gfx::GLDRAWABLE_NEEDS_FRAMEBUFFER && !defaultFB)
	{
		glContext().setCurrentContext({});
		GLuint fb;
		glGenFramebuffers(1, &fb);
		log.info("created default framebuffer:{}", fb);
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
		log.info("init FBO:{:X}", fbo);
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

void GLRendererTask::doPreDraw(Window &win, WindowDrawParams winParams, DrawParams &params) const
{
	if(!context) [[unlikely]]
	{
		log.warn("draw() called without context");
		return;
	}
	assumeExpr(winData(win).drawable);
	if(params.asyncMode == DrawAsyncMode::AUTO)
	{
		params.asyncMode = DrawAsyncMode::PRESENT;
	}
	if(winParams.needsSync) [[unlikely]]
	{
		params.asyncMode = DrawAsyncMode::NONE;
	}
}

RendererTask::operator bool() const
{
	return GLTask::operator bool();
}

void GLRendererTask::updateDrawable(Drawable drawable, IRect viewportRect, int swapInterval)
{
	context.setCurrentDrawable(drawable);
	context.setSwapInterval(swapInterval);
	glViewport(viewportRect.x, viewportRect.y, viewportRect.x2, viewportRect.y2);
}

void RendererTask::updateDrawableForSurfaceChange(Window &win, WindowSurfaceChange change)
{
	auto &data = winData(win);
	auto &drawable = data.drawable;
	switch(change.action)
	{
		case WindowSurfaceChange::Action::CREATED:
			r->makeWindowDrawable(*this, win, data.bufferConfig, data.colorSpace);
			return;
		case WindowSurfaceChange::Action::CHANGED:
			if(change.flags.surfaceResized)
			{
				GLTask::run(
					[this, drawable = (Drawable)drawable, v = data.viewportRect, swapInterval = data.swapInterval]()
					{
						// reset and clear the drawable
						updateDrawable(drawable, v, swapInterval);
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
					});
			}
			return;
		case WindowSurfaceChange::Action::DESTROYED:
			destroyDrawable(drawable);
			return;
	}
}

void RendererTask::setPresentMode(Window &win, PresentMode mode)
{
	if(!GLManager::hasSwapInterval)
		return;
	auto swapInterval = renderer().toSwapInterval(win, mode);
	auto &data = winData(win);
	if(data.swapInterval == swapInterval)
		return;
	data.swapInterval = swapInterval;
	if(!data.drawable)
		return;
	log.info("setting swap interval:{} for drawable:{}", swapInterval, Drawable(data.drawable));
	GLTask::run([this, drawable = Drawable(data.drawable), v = data.viewportRect, swapInterval]()
	{
		updateDrawable(drawable, v, swapInterval);
	});
}

void RendererTask::setDefaultViewport(Window &win, Viewport v)
{
	renderer().setDefaultViewport(win, v);
	auto &data = winData(win);
	GLTask::run(
		[drawable = (Drawable)data.drawable, v = v.asYUpRelRect()]()
		{
			// update viewport if drawable is currently in use
			if(GLManager::hasCurrentDrawable(drawable))
			{
				glViewport(v.x, v.y, v.x2, v.y2);
			}
		});
}

void GLRendererTask::destroyDrawable(GLDrawable &drawable)
{
	if(!drawable)
		return;
	GLTask::runSync(
		[this, drawable = (Drawable)drawable](TaskContext)
		{
			// unset the drawable if it's currently in use
			if(GLManager::hasCurrentDrawable(drawable))
				context.setCurrentDrawable({});
		});
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
	ctx.notifySemaphore();
	if(!Config::Gfx::OPENGL_ES && !support.hasVAOFuncs())
	{
		log.debug("creating global VAO for testing non-VAO code paths");
		GLuint name;
		support.glGenVertexArrays(1, &name);
		support.glBindVertexArray(name);
	}
	if(!support.hasVAOFuncs())
	{
		runGLCheckedVerbose([&]()
		{
			glEnableVertexAttribArray(VATTR_POS);
		}, "glEnableVertexAttribArray(VATTR_POS)");
	}
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
	auto currentCtx = GLManager::currentContext();
	if(currentCtx != glContext()) [[unlikely]]
	{
		bug_unreachable("expected GL context:%p but current is:%p", (NativeGLContext)glContext(), currentCtx);
	}
}

SyncFence RendererTask::addSyncFence()
{
	if(!r->support.hasSyncFences())
		return {}; // no-op
	GLsync sync{};
	runSync(
		[&support = r->support, &sync](TaskContext ctx)
		{
			sync = support.fenceSync(ctx.glDisplay);
		});
	return sync;
}

void RendererTask::deleteSyncFence(SyncFence fence)
{
	if(!fence.sync)
		return;
	assumeExpr(r->support.hasSyncFences());
	const bool canPerformInCurrentThread = Config::GL_PLATFORM_EGL;
	if(canPerformInCurrentThread)
	{
		auto dpy = renderer().glDisplay();
		renderer().support.deleteSync(dpy, fence.sync);
	}
	else
	{
		GLTask::run(
			[&support = r->support, sync = fence.sync](TaskContext ctx)
			{
				support.deleteSync(ctx.glDisplay, sync);
			});
	}
}

void RendererTask::clientWaitSync(SyncFence fence, int flags, std::chrono::nanoseconds timeout)
{
	if(!fence.sync)
		return;
	assumeExpr(r->support.hasSyncFences());
	const bool canPerformInCurrentThread = Config::GL_PLATFORM_EGL && !flags;
	if(canPerformInCurrentThread)
	{
		//log.debug("waiting on sync:{} flush:{} timeout:{}", fence.sync, flags & 1 ? "yes" : "no", timeout);
		auto dpy = renderer().glDisplay();
		renderer().support.clientWaitSync(dpy, fence.sync, 0, timeout.count());
		renderer().support.deleteSync(dpy, fence.sync);
	}
	else
	{
		runSync(
			[&support = r->support, sync = fence.sync, timeout, flags](TaskContext ctx)
			{
				support.clientWaitSync(ctx.glDisplay, sync, flags, timeout.count());
				ctx.notifySemaphore();
				support.deleteSync(ctx.glDisplay, sync);
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
	GLTask::run(
		[&support = r->support, sync = fence.sync](TaskContext ctx)
		{
			support.waitSync(ctx.glDisplay, sync);
			support.deleteSync(ctx.glDisplay, sync);
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
	run(
		[]()
		{
			glReleaseShaderCompiler();
		});
}

void RendererTask::setDebugOutput(bool on)
{
	if(!renderer().support.hasDebugOutput || debugEnabled == on)
	{
		return;
	}
	log.info("set context:{} debug output:{}", (NativeGLContext)glContext(), on ? "on" : "off");
	debugEnabled = on;
	run(
		[&support = renderer().support, on]()
		{
			support.setGLDebugOutput(on);
		});
}

ThreadId RendererTask::threadId() const { return threadId_; }

RendererCommands GLRendererTask::makeRendererCommands(GLTask::TaskContext taskCtx, bool manageSemaphore,
	bool notifyWindowAfterPresent, Window &win)
{
	initDefaultFramebuffer();
	auto &drawable = winData(win).drawable;
	RendererCommands cmds{*static_cast<RendererTask*>(this),
		notifyWindowAfterPresent ? &win : nullptr, drawable, winData(win).viewportRect, taskCtx.glDisplay,
		glContext(), manageSemaphore ? taskCtx.semaphorePtr : nullptr};
	if(manageSemaphore)
		taskCtx.markSemaphoreNotified(); // semaphore will be notified in RendererCommands::present()
	return cmds;
}

}
