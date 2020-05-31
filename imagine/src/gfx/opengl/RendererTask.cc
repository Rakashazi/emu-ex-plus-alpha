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
#include <imagine/gfx/Gfx.hh>
#include <imagine/base/Base.hh>
#include <imagine/base/Screen.hh>
#include <imagine/thread/Thread.hh>
#include "private.hh"

#ifndef GL_BACK_LEFT
#define GL_BACK_LEFT				0x0402
#endif

#ifndef GL_BACK_RIGHT
#define GL_BACK_RIGHT				0x0403
#endif

#ifndef GL_TIMEOUT_IGNORED
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFFull
#endif

#ifndef GL_SYNC_GPU_COMMANDS_COMPLETE
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#endif

namespace Gfx
{

static constexpr int ON_RESUME_PRIORITY = -110;
static constexpr int ON_EXIT_PRIORITY = 110;

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
	#ifdef CONFIG_GLDRAWABLE_NEEDS_FRAMEBUFFER
	if(!defaultFB)
	{
		glCtx.setCurrent(Base::GLDisplay::getDefault(), glCtx, {});
		glGenFramebuffers(1, &defaultFB);
		logMsg("created default framebuffer:%u", defaultFB);
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFB);
	}
	#endif
}

GLuint GLRendererTask::bindFramebuffer(Texture &tex)
{
	if(unlikely(!fbo))
	{
		glGenFramebuffers(1, &fbo);
		logMsg("init FBO:0x%X", fbo);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.texName(), 0);
	return fbo;
}

void GLRendererTask::replyHandler(Renderer &r, GLRendererTask::ReplyMessage msg)
{
	switch(msg.reply)
	{
		case Reply::DRAW_FINISHED:
		{
			//logDMsg("draw presented");
			FrameParams frameParams{msg.timestamp, drawTimestamp, Base::Screen::screen(0)->frameTime()};
			onFrame.runAll([&](Base::OnFrameDelegate del){ return del(frameParams); });
			drawTimestamp = msg.timestamp;
			return;
		}
		default:
		{
			logWarn("clearing RenderThreadReplyMessage value:%d", (int)msg.reply);
			return;
		}
	}
}

bool GLRendererTask::commandHandler(decltype(commandPort)::Messages messages, Base::GLDisplay glDpy, bool ownsThread)
{
	int msgs = 0, draws = 0;
	for(auto msg = messages.get(); msg; msg = messages.get())
	{
		msgs++;
		//logMsg("command pipe data");
		switch(msg.command)
		{
			bcase Command::DRAW:
			{
				//logMsg("got draw command");
				draws++;
				auto &drawArgs = msg.args.draw;
				assumeExpr(drawArgs.del);
				assumeExpr(drawArgs.winPtr);
				drawArgs.del(drawArgs.drawable, *drawArgs.winPtr, drawArgs.fence, {*static_cast<RendererTask*>(this), glDpy, msg.semAddr});
				if(onFrame.size())
				{
					auto now = IG::steadyClockTimestamp();
					replyPort.send({Reply::DRAW_FINISHED, now});
				}
				drawArgs.winPtr->deferredDrawComplete();
			}
			bcase Command::RUN_FUNC:
			{
				assumeExpr(msg.args.runFunc.func);
				msg.args.runFunc.func(*static_cast<RendererTask*>(this));
				if(msg.semAddr)
				{
					msg.semAddr->notify();
				}
			}
			bcase Command::EXIT:
			{
				if(ownsThread)
				{
					Base::GLContext::setCurrent(glDpy, {}, {});
					eventLoopRunning = false;
					Base::EventLoop::forThread().stop();
				}
				else
				{
					// only unset the drawable
					Base::GLContext::setCurrent(glDpy, glCtx, {});
				}
				assumeExpr(msg.semAddr);
				msg.semAddr->notify();
				return false;
			}
			bdefault:
			{
				logWarn("unknown GLRendererTask::CommandMessage value:%d", (int)msg.command);
			}
		}
	}
	if(msgs > 1)
	{
		//logDMsg("read %d messages, %d draws", msgs, draws);
	}
	return true;
}

RendererTask::RendererTask(Renderer &r): r{r} {}

void RendererTask::start()
{
	if(unlikely(!onResume))
	{
		onResume =
			[this](bool focused)
			{
				start();
				return true;
			};
		Base::addOnResume(onResume, ON_RESUME_PRIORITY);
		onExit =
			[this](bool backgrounded)
			{
				stop();
				return true;
			};
		Base::addOnExit(onExit, ON_EXIT_PRIORITY);
	}
	r.addOnExitHandler();
	if constexpr(Config::envIsIOS)
		r.setIOSDrawableDelegates();
	if((bool)glCtx)
	{
		//logWarn("render thread already started");
		return;
	}
	if(r.useSeparateDrawContext)
	{
		//logMsg("setting up rendering separate GL thread");
		std::error_code ec{};
		glCtx = {r.glDpy, r.makeKnownGLContextAttributes(), r.gfxBufferConfig, r.gfxResourceContext, ec};
		if(!glCtx)
		{
			logErr("error creating context");
		}
		r.finishContextCreation(glCtx);
		#ifndef NDEBUG
		r.drawContextDebug = false;
		#endif
		IG::makeDetachedThreadSync(
			[this](auto &sem)
			{
				auto glDpy = Base::GLDisplay::getDefault(glAPI);
				assumeExpr(glDpy);
				auto eventLoop = Base::EventLoop::makeForThread();
				commandPort.attach(eventLoop,
					[this, glDpy](auto msgs)
					{
						return commandHandler(msgs, glDpy, true);
					});
				sem.notify();
				logMsg("starting render task event loop");
				eventLoopRunning = true;
				eventLoop.run(eventLoopRunning);
				commandPort.detach();
				logMsg("render task thread finished");
			});
	}
	else
	{
		//logMsg("setting up rendering in main GL thread");
		glCtx = r.gfxResourceContext;
		r.runGLTaskSync(
			[this]()
			{
				logMsg("starting render task in main GL thread");
				auto glDpy = Base::GLDisplay::getDefault();
				commandPort.attach(
					[this, glDpy](auto msgs)
					{
						return commandHandler(msgs, glDpy, false);
					});
			});
	}
	replyPort.attach(
		[this](auto msgs)
		{
			auto msg = msgs.get();
			replyHandler(r, msg);
			return true;
		});
}

void RendererTask::draw(DrawableHolder &drawableHolder, Base::Window &win, Base::Window::DrawParams winParams, DrawParams params, DrawDelegate del)
{
	if(unlikely(!glCtx))
	{
		logWarn("drawing without starting render task");
		return;
	}
	if(winParams.wasResized())
	{
		if(win == Base::mainWindow())
		{
			if(!Config::SYSTEM_ROTATES_WINDOWS)
			{
				r.setProjectionMatrixRotation(orientationToGC(win.softOrientation()));
				Base::setOnDeviceOrientationChanged(
					[&renderer = r, &win](Base::Orientation newO)
					{
						auto oldWinO = win.softOrientation();
						if(win.requestOrientationChange(newO))
						{
							renderer.animateProjectionMatrixRotation(orientationToGC(oldWinO), orientationToGC(newO));
						}
					});
			}
			else if(Config::SYSTEM_ROTATES_WINDOWS && !Base::Window::systemAnimatesRotation())
			{
				Base::setOnSystemOrientationChanged(
					[&renderer = r](Base::Orientation oldO, Base::Orientation newO) // TODO: parameters need proper type definitions in API
					{
						const Angle orientationDiffTable[4][4]
						{
							{0, angleFromDegree(90), angleFromDegree(-180), angleFromDegree(-90)},
							{angleFromDegree(-90), 0, angleFromDegree(90), angleFromDegree(-180)},
							{angleFromDegree(-180), angleFromDegree(-90), 0, angleFromDegree(90)},
							{angleFromDegree(90), angleFromDegree(-180), angleFromDegree(-90), 0},
						};
						auto rotAngle = orientationDiffTable[oldO][newO];
						logMsg("animating from %d degrees", (int)angleToDegree(rotAngle));
						renderer.animateProjectionMatrixRotation(rotAngle, 0.);
					});
			}
		}
	}
	if(unlikely(!drawableHolder.drawable()))
	{
		drawableHolder.makeDrawable(r, win);
	}
	if(unlikely(winParams.needsSync()))
	{
		params.setAsyncMode(AsyncMode::NONE);
	}
	SyncFence fence = params.fenceMode() == FenceMode::RESOURCE ? r.addResourceSyncFence() : SyncFence();
	IG::Semaphore drawSem{0};
	{
		#ifdef CONFIG_GFX_RENDERER_TASK_DRAW_LOCK
		auto lock = std::unique_lock<std::mutex>{drawMutex};
		drawCondition.wait(lock, [this](){ return canDraw; });
		#endif
		commandPort.send(
			{
				Command::DRAW,
				del,
				drawableHolder.drawable(),
				win,
				fence.sync,
				params.asyncMode() == AsyncMode::PRESENT ? &drawSem : nullptr
			});
	}
	//logMsg("wrote render thread draw command");
	if(params.asyncMode() == AsyncMode::PRESENT)
	{
		//logMsg("waiting for draw to present");
		drawSem.wait();
	}
	else if(unlikely(params.asyncMode() == AsyncMode::NONE))
	{
		//logMsg("waiting for draw to finish");
		waitForDrawFinished();
	}
}

#ifdef CONFIG_GFX_RENDERER_TASK_DRAW_LOCK
void RendererTask::lockDraw()
{
	assumeExpr(canDraw);
	{
		auto lock = std::scoped_lock<std::mutex>{drawMutex};
		canDraw = false;
	}
	waitForDrawFinished();
}

void RendererTask::unlockDraw()
{
	assumeExpr(!canDraw);
	{
		auto lock = std::scoped_lock<std::mutex>{drawMutex};
		canDraw = true;
	}
	drawCondition.notify_one();
}
#endif

void RendererTask::waitForDrawFinished()
{
	runSync([](RendererTask &){});
}

void RendererTask::run(RenderTaskFuncDelegate func, IG::Semaphore *semAddr)
{
	if(!glCtx)
		return;
	commandPort.send({Command::RUN_FUNC, func, semAddr});
}

void RendererTask::runSync(RenderTaskFuncDelegate func)
{
	if(!glCtx)
		return;
	IG::Semaphore sem{0};
	run(func, &sem);
	sem.wait();
}

void RendererTask::acquireFenceAndWait(Gfx::SyncFence &fenceVar)
{
	if(!r.useSeparateDrawContext)
		return;
	Gfx::SyncFence fence{};
	runSync([&fence, &fenceVar](RendererTask &)
	{
		fence = fenceVar;
		fenceVar = {};
	});
	//logDMsg("waiting on fence:%p", fence.sync);
	renderer().waitSync(fence);
}

bool RendererTask::addOnFrame(Base::OnFrameDelegate del, int priority)
{
	if(!onFrame.size())
	{
		// reset time-stamp when first delegate is added
		drawTimestamp = {};
	}
	return onFrame.add(del, priority);
}

bool RendererTask::removeOnFrame(Base::OnFrameDelegate del)
{
	return onFrame.remove(del);
}

void RendererTask::stop()
{
	if(!glCtx)
	{
		return;
	}
	IG::Semaphore sem{0};
	commandPort.send({Command::EXIT, &sem});
	sem.wait();
	commandPort.clear();
	if(!r.useSeparateDrawContext)
	{
		r.runGLTaskSync(
			[this]()
			{
				commandPort.detach();
			});
	}
	replyPort.clear();
	replyPort.detach();
	destroyContext(r.useSeparateDrawContext, r.glDpy);
}

void RendererTask::updateDrawableForSurfaceChange(DrawableHolder &drawableHolder, Base::Window::SurfaceChange change)
{
	if(change.destroyed())
	{
		destroyDrawable(drawableHolder);
	}
	else if(change.reset())
	{
		resetDrawable = true;
	}
}

void RendererTask::destroyDrawable(DrawableHolder &drawableHolder)
{
	waitForDrawFinished();
	drawableHolder.destroyDrawable(r);
}

Base::FrameTime RendererTask::lastDrawTimestamp() const
{
	return drawTimestamp;
}

void GLRendererTask::destroyContext(bool useSeparateDrawContext, Base::GLDisplay dpy)
{
	if(!glCtx)
		return;
	if(!useSeparateDrawContext)
	{
		glCtx = {};
		return;
	}
	glCtx.deinit(dpy);
	#ifndef CONFIG_GFX_OPENGL_ES
	streamVAO = 0;
	streamVBO = {};
	streamVBOIdx = 0;
	#endif
	fbo = 0;
	#ifdef CONFIG_GLDRAWABLE_NEEDS_FRAMEBUFFER
	defaultFB = 0;
	#endif
	contextInitialStateSet = false;
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

void GLRendererTask::initialCommands(RendererCommands &cmds)
{
	if(likely(contextInitialStateSet))
		return;
	if(cmds.renderer().support.hasVBOFuncs)
		initVBOs();
	#ifndef CONFIG_GFX_OPENGL_ES
	if(cmds.renderer().useStreamVAO)
		initVAO();
	#endif
	runGLCheckedVerbose([&]()
	{
		glEnableVertexAttribArray(VATTR_POS);
	}, "glEnableVertexAttribArray(VATTR_POS)");
	cmds.setClearColor(0, 0, 0);
	contextInitialStateSet = true;
}

GLRendererDrawTask::GLRendererDrawTask(RendererTask &task, Base::GLDisplay glDpy, IG::Semaphore *semAddr):
	task{task}, glDpy{glDpy}, semAddr{semAddr}
{}

void GLRendererDrawTask::setCurrentDrawable(Drawable drawable)
{
	auto glCtx = task.glContext();
	auto &r = task.renderer();
	assert(glCtx);
	if(unlikely(glCtx != Base::GLContext::current(glDpy)))
	{
		//logMsg("restoring context");
		glCtx.setCurrent(glDpy, glCtx, drawable);
	}
	else if(task.handleDrawableReset() || !Base::GLContext::isCurrentDrawable(glDpy, drawable))
	{
		glCtx.setDrawable(glDpy, drawable, glCtx);
		if(!r.useSeparateDrawContext && r.support.hasDrawReadBuffers() && drawable)
		{
			//logMsg("specifying draw/read buffers");
			const GLenum back = Config::Gfx::OPENGL_ES_MAJOR_VERSION ? GL_BACK : GL_BACK_LEFT;
			r.support.glDrawBuffers(1, &back);
			r.support.glReadBuffer(GL_BACK);
		}
	}
}

void GLRendererDrawTask::present(Drawable win)
{
	task.glContext().present(glDpy, win, task.glContext());
}

GLuint GLRendererDrawTask::bindFramebuffer(Texture &tex)
{
	if(!tex)
		return 0;
	return task.bindFramebuffer(tex);
}

GLuint GLRendererDrawTask::getVBO()
{
	return task.getVBO();
}

GLuint GLRendererDrawTask::defaultFramebuffer() const
{
	return task.defaultFBO();
}

void GLRendererDrawTask::notifySemaphore()
{
	if(semAddr)
	{
		semAddr->notify();
	}
}

RendererCommands RendererDrawTask::makeRendererCommands(Drawable drawable, Viewport viewport, Mat4 projMat)
{
	task.initDefaultFramebuffer();
	RendererCommands cmds{*this, drawable};
	task.initialCommands(cmds);
	cmds.setViewport(viewport);
	cmds.setProjectionMatrix(projMat);
	return cmds;
}

void RendererDrawTask::verifyCurrentContext() const
{
	if(!Config::DEBUG_BUILD)
		return;
	auto currentCtx = Base::GLContext::current(glDpy);
	if(unlikely(task.glContext() != currentCtx))
	{
		bug_unreachable("expected GL context:%p but current is:%p", task.glContext().nativeObject(), currentCtx.nativeObject());
	}
}

void RendererDrawTask::notifyCommandsFinished()
{
	notifySemaphore();
}

Renderer &RendererDrawTask::renderer() const
{
	return task.renderer();
}

}
