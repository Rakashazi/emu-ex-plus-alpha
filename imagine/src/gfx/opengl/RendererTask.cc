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
#include <imagine/thread/Thread.hh>
#include "private.hh"

#ifndef GL_BACK_LEFT
#define GL_BACK_LEFT				0x0402
#endif

#ifndef GL_BACK_RIGHT
#define GL_BACK_RIGHT				0x0403
#endif

#ifndef GL_TIMEOUT_EXPIRED
#define GL_TIMEOUT_EXPIRED 0x911B
#endif

#ifndef GL_TIMEOUT_IGNORED
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFFull
#endif

namespace Gfx
{

static constexpr int ON_EXIT_PRIORITY = -100;

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
			onDrawFinished.runAll([=](DrawFinishedDelegate del){ return del(msg.channel); });
			return;
		}
		default:
		{
			logWarn("clearing RenderThreadReplyMessage value:%d", (int)msg.reply);
			return;
		}
	}
}

void GLRendererTask::waitForCommandFinished(Renderer &r)
{
	ReplyMessage msg{};
	while(replyPipe.read(&msg, sizeof(msg)))
	{
		if(msg.reply == Reply::COMMAND_FINISHED)
			return;
		else
			replyHandler(r, msg);
	}
}

void GLRendererTask::waitForDrawFinished(Renderer &r)
{
	ReplyMessage msg{};
	while(replyPipe.read(&msg, sizeof(msg)))
	{
		replyHandler(r, msg);
		if(msg.reply == Reply::DRAW_FINISHED)
		{
			return;
		}
	}
}

bool GLRendererTask::commandHandler(Base::Pipe &pipe, Base::GLDisplay glDpy, bool ownsThread)
{
	auto c = channels;
	assumeExpr(c);
	CommandMessage::Args::DrawArgs drawArg[c];
	iterateTimes(c, i)
	{
		drawArg[i].del = {};
	}
	bool haltDraw = false;
	bool releaseContext = false;
	while(pipe.hasData())
	{
		//logMsg("command pipe data");
		auto msg = pipe.readNoErr<CommandMessage>();
		switch(msg.command)
		{
			bcase Command::DRAW:
			{
				//logMsg("got draw command");
				assumeExpr(msg.channel < channels);
				drawArg[msg.channel] = msg.args.draw;
			}
			bcase Command::HALT_DRAWING:
			{
				haltDraw = true;
			}
			bcase Command::EXIT:
			{
				releaseContext = true;
			}
			bdefault:
			{
				logWarn("unknown RenderThreadCommandMessage value:%d", (int)msg.command);
			}
		}
	}
	if(releaseContext)
	{
		if(ownsThread)
		{
			Base::GLContext::setCurrent(glDpy, {}, {});
			Base::EventLoop::forThread().stop();
		}
		else
		{
			// only unset the drawable
			Base::GLContext::setCurrent(glDpy, glCtx, {});
			replyPipe.write(ReplyMessage{Reply::COMMAND_FINISHED});
		}
		return false;
	}
	if(haltDraw)
	{
		replyPipe.write(ReplyMessage{Reply::COMMAND_FINISHED});
		return true;
	}
	iterateTimes(c, i)
	{
		auto arg = drawArg[i];
		if(!arg.del)
			continue;
		arg.del(arg.drawable, *arg.winPtr, {*static_cast<RendererTask*>(this), glDpy});
		if(onDrawFinished.size())
		{
			replyPipe.write(ReplyMessage{Reply::DRAW_FINISHED, (ChannelInt)i});
		}
	}
	return true;
}

RendererTask::RendererTask(Renderer &r): r{r} {}

void RendererTask::start(uint channels_)
{
	if(!glCtx)
	{
		replyPipe.addToEventLoop({},
			[this](Base::Pipe &pipe)
			{
				auto msg = pipe.readNoErr<ReplyMessage>();
				replyHandler(r, msg);
				return true;
			});
	}
	if(!onResume)
	{
		onResume =
			[this](bool focused)
			{
				start();
				return true;
			};
		Base::addOnResume(onResume);
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
		logWarn("render thread already started");
		return;
	}
	if(channels_)
	{
		channels = channels_;
	}
	if(!channels)
		channels = 1;
	if(r.useSeparateDrawContext)
	{
		//logMsg("setting up rendering separate GL thread");
		std::error_code ec{};
		glCtx = {r.glDpy, r.makeKnownGLContextAttributes(), r.gfxBufferConfig, r.gfxResourceContext, ec};
		if(!glCtx)
		{
			logErr("error creating context");
		}
		IG::makeDetachedThread(
			[this]()
			{
				auto glDpy = Base::GLDisplay::getDefault();
				#ifdef CONFIG_GFX_OPENGL_ES
				if(!Base::GLContext::bindAPI(Base::GLContext::OPENGL_ES_API))
				{
					bug_unreachable("unable to bind GLES API");
				}
				#endif
				auto eventLoop = Base::EventLoop::makeForThread();
				commandPipe.addToEventLoop(eventLoop,
					[this, glDpy](Base::Pipe &pipe)
					{
						return commandHandler(pipe, glDpy, true);
					});
				replyPipe.write(ReplyMessage{Reply::COMMAND_FINISHED});
				logMsg("starting render task event loop");
				eventLoop.run();
				logMsg("render task exit");
				commandPipe.removeFromEventLoop();
				replyPipe.write(ReplyMessage{Reply::COMMAND_FINISHED});
			});
		waitForCommandFinished(r);
	}
	else
	{
		//logMsg("setting up rendering in main GL thread");
		glCtx = r.gfxResourceContext;
		r.runGLTaskSync(
			[this]()
			{
				logMsg("starting render task");
				auto glDpy = Base::GLDisplay::getDefault();
				commandPipe.addToEventLoop({},
					[this, glDpy](Base::Pipe &pipe)
					{
						return commandHandler(pipe, glDpy, false);
					});
			});
	}
}

void RendererTask::draw(DrawableHolder &drawableHolder, Base::Window &win, Base::Window::DrawParams params, DrawDelegate del, uint channel)
{
	if(unlikely(!glCtx))
	{
		logWarn("drawing without starting render task");
		return;
	}
	if(params.wasResized())
	{
		if(win == Base::mainWindow())
		{
			if(!Config::SYSTEM_ROTATES_WINDOWS)
			{
				r.setProjectionMatrixRotation(orientationToGC(win.softOrientation()));
				Base::setOnDeviceOrientationChanged(
					[&renderer = r, &win](uint newO)
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
					[&renderer = r](uint oldO, uint newO) // TODO: parameters need proper type definitions in API
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
	if(params.needsSync())
	{
		onDrawFinished.add([](uint){ return false; }, 0);
	}
	commandPipe.write(
		CommandMessage
		{
			Command::DRAW,
			(ChannelInt)channel,
			del,
			drawableHolder.drawable(),
			win
		});
	//logMsg("wrote render thread draw command");
	if(params.needsSync())
	{
		logMsg("waiting for draw to finish");
		waitForDrawFinished(r);
	}
}

bool RendererTask::addOnDrawFinished(DrawFinishedDelegate del, int priority)
{
	return onDrawFinished.add(del, priority);
}

bool RendererTask::removeOnDrawFinished(DrawFinishedDelegate del)
{
	return onDrawFinished.remove(del);
}

void RendererTask::haltDrawing()
{
	if(!glCtx)
		return;
	commandPipe.write(CommandMessage{Command::HALT_DRAWING});
	waitForCommandFinished(r);
}

void RendererTask::stop()
{
	if(!glCtx)
	{
		return;
	}
	commandPipe.write(CommandMessage{Command::EXIT});
	waitForCommandFinished(r);
	if(!r.useSeparateDrawContext)
	{
		r.runGLTaskSync(
			[this]()
			{
				commandPipe.removeFromEventLoop();
			});
	}
	replyPipe.removeFromEventLoop();
	destroyContext(r.useSeparateDrawContext, r.glDpy);
}

void RendererTask::updateDrawableForSurfaceChange(DrawableHolder &drawableHolder, Base::Window::SurfaceChange change)
{
	haltDrawing();
	if(change.destroyed())
	{
		drawableHolder.destroyDrawable(r);
	}
	else if(change.reset())
	{
		resetDrawable = true;
	}
}

Renderer &RendererTask::renderer() const
{
	return r;
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

GLRendererDrawTask::GLRendererDrawTask(RendererTask &task, Base::GLDisplay glDpy):
	task{task}, glDpy{glDpy}
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

RendererCommands RendererDrawTask::makeRendererCommands(Drawable drawable, Viewport viewport, Mat4 projMat)
{
	task.initDefaultFramebuffer();
	RendererCommands cmds{*this, drawable};
	if(renderer().support.hasVBOFuncs)
		task.initVBOs();
	#ifndef CONFIG_GFX_OPENGL_ES
	if(renderer().useStreamVAO)
		task.initVAO();
	#endif
	runGLCheckedVerbose([&]()
	{
		glEnableVertexAttribArray(VATTR_POS);
	}, "glEnableVertexAttribArray(VATTR_POS)");
	cmds.setViewport(viewport);
	cmds.setProjectionMatrix(projMat);
	return cmds;
}

void RendererDrawTask::waitSync(SyncFence fence)
{
	if(!fence.sync)
		return;
	assert(renderer().useSeparateDrawContext);
	if(renderer().support.hasSyncFences())
	{
		renderer().support.glWaitSync(fence.sync, 0, GL_TIMEOUT_IGNORED);
		renderer().support.glDeleteSync(fence.sync);
	}
	else
	{
		renderer().runGLTaskSync(
			[]()
			{
				glFinish();
			}, task.syncSemaphoreAddr());
	}
}

void RendererDrawTask::verifyCurrentContext()
{
	if(!Config::DEBUG_BUILD)
		return;
	auto currentCtx = Base::GLContext::current(glDpy);
	if(unlikely(task.glContext() != currentCtx))
	{
		bug_unreachable("expected GL context:%p but current is:%p", task.glContext().nativeObject(), currentCtx.nativeObject());
	}
}

Renderer &RendererDrawTask::renderer() const
{
	return task.renderer();
}

}
